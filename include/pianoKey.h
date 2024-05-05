#ifndef PIANOKEY_H
#define PIANOKEY_H

#include <stdio.h>

#include "util.h"


#define MULTIPLEX_NUM    (7)
#define MULTIPLEX_CH_NUM (14)
#define EXTRATONENUM     (4)

// クラスの定義
class PianoKey {
 public:
  void init();
  void process(uint32_t multiplx, uint32_t multiplxCH);

  typedef struct
  {
    seqID_t  seqID;
    uint32_t start;
    uint32_t end;
    uint32_t upper;
    uint32_t lower;
    uint32_t count;
    float    volume;
    float    freq;
  } key_st;

  key_st key[MULTIPLEX_NUM][MULTIPLEX_CH_NUM];

 private:
  void polling(uint32_t multiplx, uint32_t multiplxCH);
  void stateLower(uint32_t multiplx, uint32_t multiplxCH);
  void state(uint32_t multiplx, uint32_t multiplxCH);
};

#endif /* PIANOKEY_H */