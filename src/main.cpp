#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "PianoKey.h"
#include "multiplex.h"
#include "pwm.h"
#include "util.h"

hw_timer_t                *timer = NULL;
volatile SemaphoreHandle_t timerSemProCPU;
portMUX_TYPE               timerMux   = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t          isrCounter = 0;
volatile uint32_t          lastIsrAt  = 0;

PianoKey  pianokey;
Multiplex multiplex;
Pwm       pwm;

const uint32_t    numNeoPix  = 3;
const uint32_t    portNeoPix = 25;
Adafruit_NeoPixel strip      = Adafruit_NeoPixel(numNeoPix, portNeoPix, NEO_GRB + NEO_KHZ800);

void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemProCPU, NULL);
}

void taskOnAppCPU(void *pvParameters) {
  while (1) {
    const float SAMPLING_RFEQ = (float)TASK_FREQ;  // 100[kHz]
    uint32_t    isrCount, isrTime, time_count;
    float       time;

    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime  = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);

    time_count = 0xfffff & isrTime;
    time       = (float)time_count / SAMPLING_RFEQ;

    // Turn on LEDs one by one

    for (int i = 0; i < numNeoPix; i++) {
      strip.setPixelColor(i, 100, 45, 0);  // Set the color of the i-th LED to red
      strip.show();                        // Update the LED strip with the new colors
      delay(100);                          // Wait for 100 milliseconds
    }

    // Turn off LEDs one by one
    for (int i = 0; i < numNeoPix; i++) {
      strip.setPixelColor(i, 0, 0, 0);  // Set the color of the i-th LED to black (turn it off)
      strip.show();                     // Update the LED strip with the new colors
      delay(100);                       // Wait for 100 milliseconds
    }
    // No less than delay(1) is needed for the ProCPU to run
    // Don't know why but ProCPU stop running without delay(1) here.
    // delay(1);
  }
}

void taskOnProCPU(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(timerSemProCPU, 0) == pdTRUE) {
      uint32_t        isrCount, isrTime, diffIsrCount;
      static uint32_t isrCountPast;

      portENTER_CRITICAL(&timerMux);
      isrCount = isrCounter;
      isrTime  = lastIsrAt;
      portEXIT_CRITICAL(&timerMux);

      diffIsrCount = isrCount - isrCountPast;
      isrCountPast = isrCount;

      static uint32_t multiplx;
      static uint32_t multiplxCH;

      if (++multiplx > MULTIPLEX_NUM - 1) {
        multiplx = 0;
        if (++multiplxCH > MULTIPLEX_CH_NUM - 1) multiplxCH = 0;
        multiplex.output(multiplxCH);
      }
      pianokey.process(multiplx, multiplxCH);
      pwm.output(isrCount, &pianokey);

      static uint32_t index;
      if ((++index % 100000) == 0) Serial.printf("%d\n", diffIsrCount);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // タスクを作る前にpinModeの設定をする必要がある
  pianokey.init();
  multiplex.init();
  pwm.init();
  strip.begin();
  strip.show();

  // Create semaphore to inform us when the timer has fired
  timerSemProCPU = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Clock count at 40MHz(=80MHz/2). By the prescaler 2 which must be in the range from 2 to 655535
  timer = timerBegin(0, PRESCALE, true);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every 10us(400 = 40,000,000Hz / 100,000Hz)
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, PRESCALED_CLOCK / TASK_FREQ, true);
  // Start an alarm
  timerAlarmEnable(timer);

  xTaskCreateUniversal(
      taskOnAppCPU,
      "taskOnAppCPU",
      8192,
      NULL,
      configMAX_PRIORITIES - 1,  // 最高優先度
      NULL,
      APP_CPU_NUM  // PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );

  xTaskCreateUniversal(
      taskOnProCPU,
      "taskOnProCPU",
      8192,
      NULL,
      configMAX_PRIORITIES - 1,  // 最高優先度
      NULL,
      PRO_CPU_NUM  // PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );

  disableCore0WDT();  // PRO_CPUのウォッチドッグ停止
}

void loop() {
  delay(1);
}
