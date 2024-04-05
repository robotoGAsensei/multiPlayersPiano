#include "pianoKey.h"

#include <esp32-hal.h>  // C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\cores\esp32
#include <stdio.h>

const uint32_t upperDIN[MULTIPLEX_NUM] = {36, 34, 32, 27, 12, 15, 16};
const uint32_t lowerDIN[MULTIPLEX_NUM] = {39, 35, 33, 14, 13, 4, 17};

const float freqList[MULTIPLEX_NUM][MULTIPLEX_CH_NUM] = {
    {36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55.000, 58.270, 61.735, 65.406, 69.296, 32.703, 34.648},
    {73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.83, 110.00, 116.54, 123.47, 130.81, 138.59, 000.00, 000.00},
    {146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 000.00, 000.00},
    {293.67, 311.13, 329.63, 349.23, 369.99, 392.00, 415.31, 440.00, 466.16, 493.88, 523.25, 554.37, 000.00, 000.00},
    {587.33, 622.25, 659.26, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77, 1046.5, 1108.7, 000.00, 000.00},
    {1174.7, 1244.5, 1318.5, 1396.9, 1480.0, 1568.0, 1661.2, 1760.0, 1864.7, 1975.5, 2093.0, 2217.5, 000.00, 000.00},
    {2349.3, 2489.0, 2637.0, 2793.8, 2960.0, 3136.0, 3322.4, 3520.0, 3729.3, 3951.1, 4186.0, 4434.9, 4698.6, 4978.0},
};

void PianoKey::init() {
  for (uint32_t multiplx = 0; multiplx < MULTIPLEX_NUM; multiplx++) {
    pinMode(upperDIN[multiplx], INPUT);
    pinMode(lowerDIN[multiplx], INPUT);
    for (uint32_t multiplxCH = 0; multiplxCH < MULTIPLEX_CH_NUM; multiplxCH++) {
      key[multiplx][multiplxCH].freq = freqList[multiplx][multiplxCH];
    }
  }
}

void PianoKey::process(uint32_t octave, uint32_t multiplxCH) {
  polling(octave, multiplxCH);
  stateLower(octave, multiplxCH);  // Uppwer接点んが機能せずLower接点のみが機能
  // state(octave, multiplxCH);//Upper接点、Lower接点の両方ともに機能
}

void PianoKey::polling(uint32_t octave, uint32_t multiplxCH) {
  key_st *ppianokey = &key[octave][multiplxCH];
  /*
    例：第0オクターブ
            upper   lower   mux_tone  key
    RE      IO36    IO39    0         key[0][0]
    RE#     IO36    IO39    1         key[0][1]
    MI      IO36    IO39    2         key[0][2]
    FA      IO36    IO39    3         key[0][3]
    FA#     IO36    IO39    4         key[0][4]
    SO      IO36    IO39    5         key[0][5]
    SO#     IO36    IO39    6         key[0][6]
    RA      IO36    IO39    7         key[0][7]
    RA#     IO36    IO39    8         key[0][8]
    SHI     IO36    IO39    9         key[0][9]
    DO      IO36    IO39    10        key[0][10]
    DO#     IO36    IO39    11        key[0][11]
    Free0   IO36    IO39    12        key[0][12]
    Free1   IO36    IO39    13        key[0][13]
    Free2   IO36    IO39    14        key[0][14]
    Free3   IO36    IO39    15        key[0][15]
  */
  ppianokey->upper = digitalRead(upperDIN[octave]);
  ppianokey->lower = digitalRead(lowerDIN[octave]);
}

void PianoKey::stateLower(uint32_t octave, uint32_t multiplxCH) {
  key_st *ppianokey = &key[octave][multiplxCH];

  if (ppianokey->lower == true)
    ppianokey->volume = 1.0;
  else
    ppianokey->volume = 0.0;
}

void PianoKey::state(uint32_t octave, uint32_t multiplxCH) {
  key_st *ppianokey = &key[octave][multiplxCH];

  switch (ppianokey->seqID) {
    case STEP00:                 /* キーが押されていない状態 */
      if (ppianokey->upper == true) { /* キーが押された */
        ppianokey->start = millis();
        ppianokey->seqID = STEP01;
      } else if ((ppianokey->upper == false) &&
                 (ppianokey->lower == false)) { /* 押してない状態を継続 */
      } else if ((ppianokey->upper == false) && (ppianokey->lower == true)) {
        /* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */
      }
      break;
    case STEP01: /* キーを途中まで押した */
      if ((ppianokey->upper == false) && (ppianokey->lower == false)) {
        ppianokey->seqID = STEP00; /* 最後まで押されずに途中でリリース */
      } else if ((ppianokey->upper == false) && (ppianokey->lower == true)) {
        /* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */
      } else if ((ppianokey->upper == true) &&
                 (ppianokey->lower == false)) { /* 途中まで押した状態を継続 */
      } else if ((ppianokey->upper == true) &&
                 (ppianokey->lower == true)) { /* 最後までキーを押した */
        ppianokey->end   = millis();
        ppianokey->seqID = STEP02;
      }
      break;
    case STEP02: /* 最後までキーを押した */
      if ((ppianokey->upper == false) &&
          (ppianokey->lower == false)) { /* 最後まで押した後にリリース */
        ppianokey->volume = 0.0;
        ppianokey->seqID  = STEP00;
      } else if ((ppianokey->upper == false) && (ppianokey->lower == true)) {
        /* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */
      } else if ((ppianokey->upper == true) && (ppianokey->lower == false)) {
        ppianokey->volume = 0.0;
        ppianokey->seqID  = STEP01; /* 途中まで押した状態に戻る */
      } else if ((ppianokey->upper == true) &&
                 (ppianokey->lower == true)) { /* 最後まで押した状態を維持*/
        ppianokey->volume = 1.0 / (1.0 + (float)ppianokey->end - (float)ppianokey->start);
      }
      break;
    case STEP03:
      break;
  }
}