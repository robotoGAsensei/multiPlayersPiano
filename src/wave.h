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

  seqID_t stt_waveID;

 private:
  uint32_t buttonONOFF(float trigger);
  uint32_t outputNeopix(float freqNeopix, uint32_t isrTime, float phaseDiff);
};

#endif /* WAVE_H */