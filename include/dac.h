#ifndef DAC_H
#define DAC_H

#include "PianoKey.h"

class Dac {
 public:
  void init(PianoKey *ppianokey);
  void output(PianoKey *ppianokey);

 private:
};

#endif /* DAC_H */