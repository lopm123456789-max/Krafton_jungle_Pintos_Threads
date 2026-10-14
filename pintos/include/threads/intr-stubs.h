#ifndef THREADS_INTR_STUBS_H
#define THREADS_INTR_STUBS_H

/* 인터럽트 스텁.
 *
 * intr-stubs.S에 있는 짧은 코드 조각들로, x86에서 가능한
 * 인터럽트 256개마다 하나씩 있음. 각각 스택을 조금 정리한 뒤
 * intr_entry()로 점프함. 자세한 내용은 intr-stubs.S 참고.
 *
 * 이 배열은 각 인터럽트 스텁의 진입점을 가리킴.
 * intr_init()이 쉽게 찾을 수 있게 하려는 것임. */
typedef void intr_stub_func (void);
extern intr_stub_func *intr_stubs[256];

#endif /* threads/intr-stubs.h */
