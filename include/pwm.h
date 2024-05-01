#ifndef PWM_H
#define PWM_H

#include <stdio.h>

#include "pianoKey.h"
#include "util.h"

class Pwm {
 public:
  void     init(void);
  void     output(uint32_t time_count, PianoKey *ppianokey, seqID_t stt_waveID);

 private:
};

#endif /* PWM_H */