#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "PianoKey.h"
#include "multiplex.h"
#include "pwm.h"
#include "sin_table.h"
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

const uint32_t numNeoPix  = 36;
const uint32_t portNeoPix = 25;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numNeoPix, portNeoPix, NEO_GRB + NEO_KHZ800);

void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemProCPU, NULL);
}

// float freqNeopix : frequency[Hz]
// uint32_t isrTime : time[ms]
// float phase      : phase diff to neigbor LED[rad] (0 to 2PI)
uint32_t outputNeopix(float freqNeopix, uint32_t isrTime, float phaseDiff) {
  const uint32_t maxDutyNeoPix = 128;  // brightness of LED : 0 to 255

  uint32_t time_count, index, phaseDiffCount, ret;
  float    time, sin_mod;

  time_count     = 0xfffff & isrTime;            // limit max number;
  time           = (float)time_count / 1000.0f;  // convert isrTime[ms] to time[s]
  phaseDiffCount = (uint32_t)(phaseDiff / USER_2PI * (float)SIN_TABLE_NUM);
  index          = (uint32_t)((float)SIN_TABLE_NUM * freqNeopix * time) + phaseDiffCount;
  index          = ((SIN_TABLE_NUM - 1) & index);  // calculate remainder by multiplexing index with TABLE_NUM
  sin_mod        = (sin_table[index] + 1) / 2.0f;
  if (sin_mod > 0.1)
    ret = (uint32_t)(maxDutyNeoPix * sin_mod);
  else
    ret = 0;

  return (ret);
}

void taskOnAppCPU(void *pvParameters) {
  while (1) {
    uint32_t isrCount, isrTime;
    float    freq;

    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime  = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);

    switch (wave.stt_waveID) {
      case SIN_WAVE:  // 3rd 5*LED =  9, 10, 11, 12, 13
        freq = 0.5f;
        strip.setPixelColor(9, 0, 0, outputNeopix(freq, isrTime, 1.2f));
        strip.setPixelColor(10, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(11, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(12, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(13, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
      case SAW_TOOTH:  // 2nd 5*LED =  4,  5,  6,  7,  8
        freq = 0.5f;
        strip.setPixelColor(4, 0, 0, outputNeopix(freq, isrTime, 1.2f));
        strip.setPixelColor(5, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(6, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(7, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(8, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
      case PWM12P5:  // 1st 4*LED =  0,  1,  2,  3
        freq = 0.5f;
        strip.setPixelColor(0, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(1, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(2, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(3, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
      case PWM25:  // 7th 6*LED = 30, 31, 32, 33, 34, 35
        freq = 0.5f;
        strip.setPixelColor(30, 0, 0, outputNeopix(freq, isrTime, 1.5f));
        strip.setPixelColor(31, 0, 0, outputNeopix(freq, isrTime, 1.2f));
        strip.setPixelColor(32, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(33, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(34, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(35, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
      case SQUARE:  // 6th 6*LED = 24, 25, 26, 27, 28, 29
        freq = 0.5f;
        strip.setPixelColor(24, 0, 0, outputNeopix(freq, isrTime, 1.5f));
        strip.setPixelColor(25, 0, 0, outputNeopix(freq, isrTime, 1.2f));
        strip.setPixelColor(26, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(27, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(28, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(29, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
      case PSEUDO_TRIANGLE:  // 5th 5*LED = 19, 20, 21, 22, 23
        freq = 0.5f;
        strip.setPixelColor(19, 0, 0, outputNeopix(freq, isrTime, 1.2f));
        strip.setPixelColor(20, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(21, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(22, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(23, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
      case TRIANGLE:  // 4th 5*LED = 14, 15, 16, 17, 18
        freq = 0.5f;
        strip.setPixelColor(14, 0, 0, outputNeopix(freq, isrTime, 1.2f));
        strip.setPixelColor(15, 0, 0, outputNeopix(freq, isrTime, 0.9f));
        strip.setPixelColor(16, 0, 0, outputNeopix(freq, isrTime, 0.6f));
        strip.setPixelColor(17, 0, 0, outputNeopix(freq, isrTime, 0.3f));
        strip.setPixelColor(18, 0, 0, outputNeopix(freq, isrTime, 0.0f));
        break;
    }

    strip.show();  // Update the LED strip with the new colors

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
