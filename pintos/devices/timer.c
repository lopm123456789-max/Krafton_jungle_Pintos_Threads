#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* 8254 타이머 칩의 하드웨어 상세는 [8254] 참고. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* OS 부팅 후 지난 타이머 tick 수. */
static int64_t ticks;

/* 타이머 tick 하나 동안 도는 루프 횟수.
   timer_calibrate()에서 초기화함. */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);

static struct list sleep_list;

/* 8254 프로그래머블 인터벌 타이머(PIT)가 초당 PIT_FREQ번
   인터럽트를 걸도록 설정하고, 해당 인터럽트를 등록함. */
void
timer_init (void) {
	/* 8254 입력 주파수를 TIMER_FREQ로 나눈 값 (반올림). */
	uint16_t count = (1193180 + TIMER_FREQ / 2) / TIMER_FREQ;

	outb (0x43, 0x34);    /* CW: 카운터 0, 하위 바이트 다음 상위 바이트, 모드 2, 2진수. */
	outb (0x40, count & 0xff);
	outb (0x40, count >> 8);

	intr_register_ext (0x20, timer_interrupt, "8254 Timer");

	list_init (&sleep_list); //초기화 
}

/* 짧은 지연에 쓰는 loops_per_tick을 측정해서 맞춤. */
void
timer_calibrate (void) {
	unsigned high_bit, test_bit;

	ASSERT (intr_get_level () == INTR_ON);
	printf ("Calibrating timer...  ");

	/* loops_per_tick을 timer tick 하나보다 짧은
	   가장 큰 2의 거듭제곱으로 어림잡음. */
	loops_per_tick = 1u << 10;
	while (!too_many_loops (loops_per_tick << 1)) {
		loops_per_tick <<= 1;
		ASSERT (loops_per_tick != 0);
	}

	/* loops_per_tick의 다음 8비트를 더 정밀하게 맞춤. */
	high_bit = loops_per_tick;
	for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
		if (!too_many_loops (high_bit | test_bit))
			loops_per_tick |= test_bit;

	printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* OS 부팅 후 지난 타이머 tick 수를 돌려줌. */
int64_t
timer_ticks (void) {
	enum intr_level old_level = intr_disable ();
	int64_t t = ticks;
	intr_set_level (old_level);
	barrier ();
	return t;
}

/* THEN 이후로 지난 타이머 tick 수를 돌려줌.
   THEN은 예전에 timer_ticks()가 돌려준 값이어야 함. */
int64_t
timer_elapsed (int64_t then) {
	return timer_ticks () - then;
}

/* 약 TICKS 타이머 tick 동안 실행을 멈춤. */
void
timer_sleep (int64_t ticks) {
	ASSERT (intr_get_level () == INTR_ON);   // ① 호출할 때는 인터럽트가 켜져 있어야 함

	if(ticks <= 0) return;

	struct thread *t = thread_current(); //현재 CPU에서 실행 중인 스레드 찾기
	t->awake_time = timer_ticks () + ticks;

	enum intr_level old_level = intr_disable ();   // ② 끄기 (이때 상태를 old_level에 저장)
	list_push_back(&sleep_list, &t->elem);		   // ③ 인터럽트가 꺼진 안전한 구간
	thread_block(); //여기서 다음 스레드로 넘어감 (깨어나면 여기서 이어서 실행)
	intr_set_level (old_level);                    // ④ 저장해 둔 인터럽트 플래그 상태로 복구
}

/* 약 MS 밀리초 동안 실행을 멈춤. */
void
timer_msleep (int64_t ms) {
	real_time_sleep (ms, 1000);
}

/* 약 US 마이크로초 동안 실행을 멈춤. */
void
timer_usleep (int64_t us) {
	real_time_sleep (us, 1000 * 1000);
}

/* 약 NS 나노초 동안 실행을 멈춤. */
void
timer_nsleep (int64_t ns) {
	real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* 타이머 통계를 출력함. */
void
timer_print_stats (void) {
	printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* 타이머 인터럽트 핸들러. */
static void
timer_interrupt (struct intr_frame *args UNUSED) {
	ticks++;
	thread_tick ();
	//sleep_list에서 깨울 스레드 찾기 -> 리스트에서 빼고, thread_unblock()
	struct list_elem *e = list_begin(&sleep_list);
	while(e != list_end(&sleep_list)){
		struct thread *t = list_entry (e, struct thread, elem); //이 e는 struct thread 안의 elem 멤버이다. 그 구조체의 시작 주소를 계산해줘 
		if(t->awake_time <= ticks){
			e = list_remove(e);
			thread_unblock(t);
		} else{
			e = list_next(e);
		}
	}
}

/* LOOPS번 반복하는 데 타이머 tick 하나보다 오래 걸리면 true,
   아니면 false. */
static bool
too_many_loops (unsigned loops) {
	/* 타이머 tick 하나를 기다림. */
	int64_t start = ticks;
	while (ticks == start)
		barrier ();

	/* LOOPS번 반복함. */
	start = ticks;
	busy_wait (loops);

	/* tick 값이 바뀌었으면 너무 오래 반복한 것임. */
	barrier ();
	return start != ticks;
}

/* 단순 루프를 LOOPS번 돌림. 짧은 지연을 만들 때 씀.

   NO_INLINE으로 표시한 이유: 코드 정렬 위치에 따라 실행 시간이
   크게 달라질 수 있음. 이 함수가 곳곳에 다르게 인라인되면
   결과를 예측하기 어려워짐. */
static void NO_INLINE
busy_wait (int64_t loops) {
	while (loops-- > 0)
		barrier ();
}

/* 약 NUM/DENOM초 동안 잠듦. */
static void
real_time_sleep (int64_t num, int32_t denom) {
	/* NUM/DENOM초를 타이머 tick으로 바꿈 (버림).

	   (NUM / DENOM) s
	   ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
	   1 s / TIMER_FREQ ticks
	   */
	int64_t ticks = num * TIMER_FREQ / denom;

	ASSERT (intr_get_level () == INTR_ON);
	if (ticks > 0) {
		/* 최소 timer tick 하나 이상 기다려야 함. 이때는 timer_sleep()을
		   씀. CPU를 다른 프로세스에게 넘겨주기 때문임. */
		timer_sleep (ticks);
	} else {
		/* 그 외에는 tick보다 짧은 시간을 정확히 재기 위해 바쁜 대기
		   루프를 씀. 오버플로가 나지 않도록 분자와 분모를
		   1000으로 줄여서 계산함. */
		ASSERT (denom % 1000 == 0);
		busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
	}
}
