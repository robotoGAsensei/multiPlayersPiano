#include "pwm.h"

#include <esp32-hal.h>
#include <stdio.h>

#include "HardwareSerial.h"
#include "pianoKey.h"
#include "pseudo_triangle_table.h"
#include "pwm12p5_table.h"
#include "pwm25_table.h"
#include "saw_tooth_table.h"
#include "sin_table.h"
#include "sqrt_table.h"
#include "square_table.h"
#include "triangle_table.h"
#include "wave.h"

const uint32_t PWMOUTPIN     = 26;
const uint32_t PWMCH         = 0;
const uint32_t PWMMAX        = 1023;
const float    SAMPLING_RFEQ = (float)TASK_FREQ;  // 100[kHz]

void Pwm::output(uint32_t time_count, PianoKey *ppianokey, seqID_t stt_waveID) {
  uint32_t waveNum = 0;
  float    result  = 0.0f;

  uint32_t index        = 0;
  uint32_t result_digit = 0;
  float    time         = 0.0f;

  // To avoid calculation error with float, limit the time_count range
  // time_count counts up to 100000 every 1s.
  // Limit the range of time_count within 16bit because multiplication with big number may iclude error.
  // The longest playable wave length is 10.49[s] (=1,048,575 / 100,000)
  time_count = 0xfffff & time_count;
  time       = (float)time_count / SAMPLING_RFEQ;

  for (uint32_t multiplx = 0; multiplx < MULTIPLEX_NUM; multiplx++) {
    for (uint32_t multiplxCH = 0; multiplxCH < MULTIPLEX_CH_NUM; multiplxCH++) {
      if (ppianokey->key[multiplx][multiplxCH].freq > 0) {
        if (ppianokey->key[multiplx][multiplxCH].volume > 0) {
          // To keep the same loudness, devide the result by the number of synthesized wave
          waveNum++;
          // theta = 2PI * f * t
          // 0 < index < TABLE_NUM corresponds to 0 < theta < 2PI
          // sin_table(index) -> sin_table(TABLE_NUM * theta / 2PI)
          // sin(theta) = sin_table(index) = sin_table(TABLE_NUM * f * t)
          index = (uint32_t)((float)SIN_TABLE_NUM * ppianokey->key[multiplx][multiplxCH].freq * time);
          // calculate remainder by multiplexing index with TABLE_NUM
          index = (SIN_TABLE_NUM - 1) & index;

          switch (stt_waveID) {
            case SIN_WAVE:
              result += sin_table[index];
              break;
            case SAW_TOOTH:
              result += saw_tooth_table[index];
              break;
            case PWM12P5:
              result += pwm12p5_table[index];
              break;
            case PWM25:
              result += pwm25_table[index];
              break;
            case SQUARE:
              result += square_table[index];
              break;
            case PSEUDO_TRIANGLE:
              result += pseudo_triangle_table[index];
              break;
            case TRIANGLE:
              result += triangle_table[index];
              break;
          }
          result /= sqrt_table[waveNum];
        }
      }
    }
  }
  result_digit = (uint32_t)((float)PWMMAX * (1.0f + result) / 2.0f);
  ledcWrite(PWMCH, result_digit);
}

void Pwm::init(void) {
  pinMode(PWMOUTPIN, OUTPUT);
  ledcSetup(PWMCH, 78125, 10);  // 78.125kHz, 10Bit(1024 resolution)
  ledcAttachPin(PWMOUTPIN, PWMCH);
  ledcWrite(PWMCH, PWMMAX / 2);  //  50%(1.7V)
}