#ifndef PWM_H
#define PWM_H

#include <stdio.h>

#include "pianoKey.h"

class Pwm {
 public:
  void init(void);
  void output(uint32_t time_count, PianoKey *ppianokey);

 private:
};

#endif /* PWM_H */