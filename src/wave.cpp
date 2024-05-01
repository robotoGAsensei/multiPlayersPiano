#include "wave.h"

#include <esp32-hal.h>
#include <stdio.h>

#include "HardwareSerial.h"
#include "pianoKey.h"

void Wave::soundSwitch(PianoKey *ppianokey) {
  float trigger = ppianokey->key[6][13].volume;

  switch (stt_waveID) {
    case SIN_WAVE:
      if (buttonONOFF(trigger) == true) stt_waveID = TRIANGLE;
      break;
    case SAW_TOOTH:
      if (buttonONOFF(trigger) == true) stt_waveID = SIN_WAVE;
      break;
    case PWM12P5:
      if (buttonONOFF(trigger) == true) stt_waveID = SAW_TOOTH;
      break;
    case PWM25:
      if (buttonONOFF(trigger) == true) stt_waveID = PWM12P5;
      break;
    case SQUARE:
      if (buttonONOFF(trigger) == true) stt_waveID = PWM25;
      break;
    case PSEUDO_TRIANGLE:
      if (buttonONOFF(trigger) == true) stt_waveID = SQUARE;
      break;
    case TRIANGLE:
      if (buttonONOFF(trigger) == true) stt_waveID = PSEUDO_TRIANGLE;
      break;
  }

  // static uint32_t indexPrint;
  // if ((++indexPrint % 10000) == 0) Serial.printf("%f %d\n", trigger, stt_waveID);
}

uint32_t Wave::buttonONOFF(float trigger) {
  static seqID_t  stt_seqID   = STEP00;
  const uint32_t  pressCounts = 10;
  static uint32_t count       = 0;
  uint32_t        ret         = 0;

  switch (stt_seqID) {
    case STEP00:
      if (trigger > 0) {
        if (++count > pressCounts) {
          count     = 0;
          stt_seqID = STEP01;
        }
      } else {
        count = 0;
      }
      break;
    case STEP01:
      if (trigger < 1) {
        if (++count > pressCounts) {
          count     = 0;
          ret       = true;
          stt_seqID = STEP00;
        }
      } else {
        count = 0;
      }
      break;
  }
  return (ret);
}

void Wave::init(void) {
  stt_waveID = SIN_WAVE;
}