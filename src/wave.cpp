#include "wave.h"

#include <Adafruit_NeoPixel.h>
#include <esp32-hal.h>
#include <stdio.h>

#include "HardwareSerial.h"
#include "pianoKey.h"
#include "sin_table.h"

const uint32_t    numNeoPix  = 36;
const uint32_t    portNeoPix = 25;
Adafruit_NeoPixel strip      = Adafruit_NeoPixel(numNeoPix, portNeoPix, NEO_GRB + NEO_KHZ800);

// float freqNeopix : frequency[Hz]
// uint32_t isrTime : time[ms]
// float phase      : phase diff to neigbor LED[rad] (0 to 2PI)
uint32_t Wave::calcNeopix(float freqNeopix, uint32_t isrTime, float phaseDiff) {
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

void Wave::lightPatternSwitch(uint32_t isrTime) {
  float freq;

  switch (stt_waveID) {
    case SIN_WAVE:  // SIN panel wired at 3rd 5*LED =  9, 10, 11, 12, 13
      freq = 0.5f;
      strip.setPixelColor(9, 0, 0, calcNeopix(freq, isrTime, 1.2f));
      strip.setPixelColor(10, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(11, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(12, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(13, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
    case SAW_TOOTH:  // SAW TOOTH panel wired at 2nd 5*LED =  4,  5,  6,  7,  8
      freq = 0.5f;
      strip.setPixelColor(4, 0, 0, calcNeopix(freq, isrTime, 1.2f));
      strip.setPixelColor(5, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(6, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(7, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(8, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
    case PWM12P5:  // PWM12p5 panel wired at 1st 4*LED =  0,  1,  2,  3
      freq = 0.5f;
      strip.setPixelColor(0, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(1, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(2, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(3, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
    case PWM25:  // PWM25 panel wired at 7th 6*LED = 30, 31, 32, 33, 34, 35
      freq = 0.5f;
      strip.setPixelColor(30, 0, 0, calcNeopix(freq, isrTime, 1.5f));
      strip.setPixelColor(31, 0, 0, calcNeopix(freq, isrTime, 1.2f));
      strip.setPixelColor(32, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(33, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(34, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(35, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
    case SQUARE:  // SQUARE panel wired at 6th 6*LED = 24, 25, 26, 27, 28, 29
      freq = 0.5f;
      strip.setPixelColor(24, 0, 0, calcNeopix(freq, isrTime, 1.5f));
      strip.setPixelColor(25, 0, 0, calcNeopix(freq, isrTime, 1.2f));
      strip.setPixelColor(26, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(27, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(28, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(29, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
    case PSEUDO_TRIANGLE:  // PSEUDE TRIANGLE panel wired at 5th 5*LED = 19, 20, 21, 22, 23
      freq = 0.5f;
      strip.setPixelColor(19, 0, 0, calcNeopix(freq, isrTime, 1.2f));
      strip.setPixelColor(20, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(21, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(22, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(23, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
    case TRIANGLE:  // TRIANGLE panel wired at 4th 5*LED = 14, 15, 16, 17, 18
      freq = 0.5f;
      strip.setPixelColor(14, 0, 0, calcNeopix(freq, isrTime, 1.2f));
      strip.setPixelColor(15, 0, 0, calcNeopix(freq, isrTime, 0.9f));
      strip.setPixelColor(16, 0, 0, calcNeopix(freq, isrTime, 0.6f));
      strip.setPixelColor(17, 0, 0, calcNeopix(freq, isrTime, 0.3f));
      strip.setPixelColor(18, 0, 0, calcNeopix(freq, isrTime, 0.0f));
      break;
  }
  strip.show();  // Update the LED strip with the new colors
}

void Wave::turnOffLED(void) {
  for (uint32_t i = 0; i < numNeoPix; i++) strip.setPixelColor(i, 0, 0, 0);
  strip.show();
}

void Wave::soundSwitch(PianoKey *ppianokey) {
  float trigger = ppianokey->key[6][13].volume;

  switch (stt_waveID) {
    case SIN_WAVE:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = TRIANGLE;
        turnOffLED();
      }
      break;
    case SAW_TOOTH:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = SIN_WAVE;
        turnOffLED();
      }
      break;
    case PWM12P5:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = SAW_TOOTH;
        turnOffLED();
      }
      break;
    case PWM25:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = PWM12P5;
        turnOffLED();
      }
      break;
    case SQUARE:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = PWM25;
        turnOffLED();
      }
      break;
    case PSEUDO_TRIANGLE:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = SQUARE;
        turnOffLED();
      }
      break;
    case TRIANGLE:
      if (buttonONOFF(trigger) == true) {
        stt_waveID = PSEUDO_TRIANGLE;
        turnOffLED();
      }
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
  strip.begin();
  strip.show();

  stt_waveID = SIN_WAVE;
}