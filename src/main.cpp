#include <Arduino.h>

#include "multiplex.h"
#include "PianoKey.h"
#include "pwm.h"
#include "util.h"

hw_timer_t                *timer = NULL;
volatile SemaphoreHandle_t timerSemAppCPU, timerSemProCPU;
portMUX_TYPE               timerMux   = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t          isrCounter = 0;
volatile uint32_t          lastIsrAt  = 0;

PianoKey  pianokey;
Multiplex multiplex;
Pwm       pwm;

void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemAppCPU, NULL);
  xSemaphoreGiveFromISR(timerSemProCPU, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}

void taskOnAppCPU(void *pvParameters) {
  static uint32_t isrCountPast;
  static uint32_t multiplxCH;

  while (1) {
    if (xSemaphoreTake(timerSemAppCPU, 0) == pdTRUE) {
      // キーボードの状態を更新しボリュームを取得する
      for (int i = 0; i < MULTIPLEX_NUM; i++) pianokey.process(i, multiplxCH);
      // マルチプレクサに出力するセレクト信号
      if (++multiplxCH > MULTIPLEX_CH_NUM - 1) multiplxCH = 0;
      multiplex.output(multiplxCH);
    }
    delay(1);
  }
}

void taskOnProCPU(void *pvParameters) {
  uint32_t        isrCount, isrTime, diffIsrCount;
  static uint32_t isrCountPast;

  while (1) {
    if (xSemaphoreTake(timerSemProCPU, 0) == pdTRUE) {
      portENTER_CRITICAL(&timerMux);
      isrCount = isrCounter;
      isrTime  = lastIsrAt;
      portEXIT_CRITICAL(&timerMux);

      diffIsrCount = isrCount - isrCountPast;
      isrCountPast = isrCount;
      pwm.output(isrCount, &pianokey);
      // If heavy process were included, this function would be less activated.
      // diffIsrCount should be 1 if the process is light enough. 1 is the desired.
      // The number greater than 1 means miss activation due to heavy process or interruption done by OS.
      // Serial.printf("%d\n",diffIsrCount);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // タスクを作る前にpinModeの設定をする必要がある
  pianokey.init();
  multiplex.init();
  pwm.init();

  // Create semaphore to inform us when the timer has fired
  timerSemAppCPU = xSemaphoreCreateBinary();
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
      "taskOnProCPU",
      8192,
      NULL,
      configMAX_PRIORITIES - 1,  // 最高優先度
      NULL,
      APP_CPU_NUM  // PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );

  xTaskCreateUniversal(
      taskOnProCPU,
      "taskOnAppCPU",
      8192,
      NULL,
      configMAX_PRIORITIES - 1,  // 最高優先度
      NULL,
      PRO_CPU_NUM  // PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );

  disableCore0WDT();  // PRO_CPUのウォッチドッグ停止
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1);
}
