#include <console.h>
#include <stdarg.h>
#include <stdio.h>
#include "devices/serial.h"
#include "devices/vga.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/synch.h"

static void vprintf_helper (char, void *);
static void putchar_have_lock (uint8_t c);

/* 콘솔 락.
   vga와 serial 계층은 각자 락을 걸기 때문에 언제 불러도 안전함.
   하지만 이 락이 있으면 printf()가 동시에 불려도 출력이 서로
   섞이지 않아서 보기 좋음. */
static struct lock console_lock;

/* 평소에는 true: 위에서 설명한 것처럼 스레드 간 출력이 섞이지
   않도록 콘솔 락을 쓰려는 것임.

   락이 동작하기 전인 부팅 초기나 콘솔 락이 초기화되기 전, 또는
   커널 panic 이후에는 false임. 앞의 경우에 락을 잡으려 하면 ASSERT
   실패가 나고, 그게 다시 panic을 일으켜 뒤의 경우가 됨. 뒤의
   경우에 panic의 원인이 버그 있는 lock_acquire() 구현이라면, 그대로
   재귀에 빠질 가능성이 큼. */
static bool use_console_lock;

/* Pintos에 디버그 출력을 많이 넣다 보면, 한 스레드가
   console_lock을 재귀적으로 잡으려는 일이 생길 수 있음. 실제 예로
   palloc_free()에 printf()를 넣었더니 이런 backtrace가 나왔음:

   lock_console()
   vprintf()
   printf()             - palloc()이 락을 다시 잡으려 함
   palloc_free()        
   schedule_tail()      - 스레드 전환 중 다른 스레드가 종료됨
   schedule()
   thread_yield()
   intr_handler()       - 타이머 인터럽트
   intr_set_level()
   serial_putc()
   putchar_have_lock()
   putbuf()
   sys_write()          - 한 프로세스가 콘솔에 쓰는 중
   syscall_handler()
   intr_handler()

   이런 문제는 디버깅하기 매우 어려워서, 깊이 카운터로 재귀 락을
   흉내 내서 문제를 피함. */
static int console_lock_depth;

/* 콘솔에 쓴 문자 수. */
static int64_t write_cnt;

/* 콘솔 락을 켬. */
void
console_init (void) {
	lock_init (&console_lock);
	use_console_lock = true;
}

/* 커널 panic이 진행 중이라고 콘솔에 알림. 이제부터 콘솔 락을
   잡으려 하지 말라는 경고임. */
void
console_panic (void) {
	use_console_lock = false;
}

/* 콘솔 통계를 출력함. */
void
console_print_stats (void) {
	printf ("Console: %lld characters output\n", write_cnt);
}

/* 콘솔 락을 획득함. */
	static void
acquire_console (void) {
	if (!intr_context () && use_console_lock) {
		if (lock_held_by_current_thread (&console_lock)) 
			console_lock_depth++; 
		else
			lock_acquire (&console_lock); 
	}
}

/* 콘솔 락을 풂. */
static void
release_console (void) {
	if (!intr_context () && use_console_lock) {
		if (console_lock_depth > 0)
			console_lock_depth--;
		else
			lock_release (&console_lock); 
	}
}

/* 현재 스레드가 콘솔 락을 갖고 있으면 true, 아니면 false. */
static bool
console_locked_by_current_thread (void) {
	return (intr_context ()
			|| !use_console_lock
			|| lock_held_by_current_thread (&console_lock));
}

/* 표준 vprintf() 함수.
   printf()와 같지만 va_list를 받음.
   출력을 vga 화면과 시리얼 포트 양쪽에 씀. */
int
vprintf (const char *format, va_list args) {
	int char_cnt = 0;

	acquire_console ();
	__vprintf (format, args, vprintf_helper, &char_cnt);
	release_console ();

	return char_cnt;
}

/* 문자열 S를 콘솔에 쓰고 줄바꿈 문자를 덧붙임. */
int
puts (const char *s) {
	acquire_console ();
	while (*s != '\0')
		putchar_have_lock (*s++);
	putchar_have_lock ('\n');
	release_console ();

	return 0;
}

/* BUFFER의 문자 N개를 콘솔에 씀. */
void
putbuf (const char *buffer, size_t n) {
	acquire_console ();
	while (n-- > 0)
		putchar_have_lock (*buffer++);
	release_console ();
}

/* C를 vga 화면과 시리얼 포트에 씀. */
int
putchar (int c) {
	acquire_console ();
	putchar_have_lock (c);
	release_console ();

	return c;
}

/* vprintf()의 도우미 함수. */
static void
vprintf_helper (char c, void *char_cnt_) {
	int *char_cnt = char_cnt_;
	(*char_cnt)++;
	putchar_have_lock (c);
}

/* C를 vga 화면과 시리얼 포트에 씀.
   필요한 경우 호출한 쪽이 이미 콘솔 락을 잡은 상태임. */
static void
putchar_have_lock (uint8_t c) {
	ASSERT (console_locked_by_current_thread ());
	write_cnt++;
	serial_putc (c);
	vga_putc (c);
}
