#ifndef FILESYS_OFF_T_H
#define FILESYS_OFF_T_H

#include <stdint.h>

/* 파일 안의 오프셋.
 * 여러 헤더가 이 정의만 필요하고 다른 건 필요 없어서
 * 헤더를 따로 분리해 둠. */
typedef int32_t off_t;

/* printf()용 형식 지정자. 예:
 * printf ("offset=%"PROTd"\n", offset); */
#define PROTd PRId32

#endif /* filesys/off_t.h */
