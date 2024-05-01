#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>

#include "pianoKey.h"
#include "util.h"

class Wave {
 public:
  void init(void);
  void LEDSwitch(void);
  void soundSwitch(PianoKey *ppianokey);
  void lightPatternSwitch(uint32_t isrTime);

  seqID_t stt_waveID;

 private:
  uint32_t buttonONOFF(float trigger);
  uint32_t calcNeopix(float freqNeopix, uint32_t isrTime, float phaseDiff);
};

#endif /* WAVE_H */