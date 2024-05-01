#include <Arduino.h>

#include "PianoKey.h"
#include "multiplex.h"
#include "pwm.h"
#include "util.h"
#include "wave.h"

hw_timer_t                *timer = NULL;
volatile SemaphoreHandle_t timerSemProCPU;
portMUX_TYPE               timerMux   = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t          isrCounter = 0;
volatile uint32_t          lastIsrAt  = 0;

PianoKey  pianokey;
Multiplex multiplex;
Pwm       pwm;
Wave      wave;

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
    uint32_t isrCount, isrTime;

    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime  = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);

    wave.lightPatternSwitch(isrTime);

    // No less than delay(1) is needed for the ProCPU to run
    // Don't know why but ProCPU stop running without delay(1) here.
    delay(1);
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
      pwm.output(isrCount, &pianokey, wave.stt_waveID);
      wave.soundSwitch(&pianokey);

      // static uint32_t index;
      // if ((++index % 100000) == 0) Serial.printf("%d %d\n", diffIsrCount, pwm.stt_waveID);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // タスクを作る前にpinModeの設定をする必要がある
  pianokey.init();
  multiplex.init();
  pwm.init();
  wave.init();

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
