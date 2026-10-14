#ifndef __LIB_ROUND_H
#define __LIB_ROUND_H

/* X를 STEP의 배수 중 가장 가까운 값으로 올림한 결과.
 * X >= 0, STEP >= 1일 때만 사용. */
#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))

/* X를 STEP으로 나눈 값을 올림한 결과.
 * X >= 0, STEP >= 1일 때만 사용. */
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

/* X를 STEP의 배수 중 가장 가까운 값으로 내림한 결과.
 * X >= 0, STEP >= 1일 때만 사용. */
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

/* DIV_ROUND_DOWN은 없음. 그냥 X / STEP이면 됨. */

#endif /* lib/round.h */
