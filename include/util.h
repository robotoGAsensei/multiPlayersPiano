#ifndef UTIL_H
#define UTIL_H

#include "util.h"

#define BASE_CLOCK      (80000000)  // 80MHz
#define PRESCALE        (2)
#define PRESCALED_CLOCK (BASE_CLOCK / PRESCALE)
#define TASK_FREQ       (100000)  // 100kHz 10us

typedef enum {
  STEP00 = 0U,
  STEP01,
  STEP02,
  STEP03,
  STEP04,
  STEP05,
  STEP06,
  STEP07,
  STEP08,
  STEP09,
  STEP10,
  STEP11,
  STEP12,
  STEP13,
  STEP14,
  STEP15,
  STEP16,
  STEP17,
  STEP18,
  STEP19,
  STEP20,
  STEP21,
  STEP22,
  STEP23,
  STEP24,
  STEP25,
  STEP26,
  STEP27,
  STEP28,
  STEP29,
  STEP30,
  LOOP,
  RUN,
  INIT,
  IDLE,
  END
} seqID_t;

#endif /* UTIL_H */