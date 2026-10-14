#include "devices/serial.h"
#include <debug.h>
#include "devices/input.h"
#include "devices/intq.h"
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* PC에서 쓰는 16550A UART의 레지스터 정의.
   16550A에는 여기 적힌 것보다 기능이 훨씬 많지만,
   필요한 건 이게 전부임.

   하드웨어 정보는 [PC16650D]를 참고. */

/* 첫 번째 시리얼 포트의 I/O 포트 기본 주소. */
#define IO_BASE 0x3f8

/* DLAB=0일 때의 레지스터. */
#define RBR_REG (IO_BASE + 0)   /* 수신 버퍼 레지스터 (읽기 전용). */
#define THR_REG (IO_BASE + 0)   /* 송신 대기 레지스터 (쓰기 전용). */
#define IER_REG (IO_BASE + 1)   /* 인터럽트 허용 레지스터. */

/* DLAB=1일 때의 레지스터. */
#define LS_REG (IO_BASE + 0)    /* 분주 래치 (하위 바이트). */
#define MS_REG (IO_BASE + 1)    /* 분주 래치 (상위 바이트). */

/* DLAB 값과 상관없는 레지스터. */
#define IIR_REG (IO_BASE + 2)   /* 인터럽트 식별 레지스터 (읽기 전용) */
#define FCR_REG (IO_BASE + 2)   /* FIFO 제어 레지스터 (쓰기 전용). */
#define LCR_REG (IO_BASE + 3)   /* 라인 제어 레지스터. */
#define MCR_REG (IO_BASE + 4)   /* 모뎀 제어 레지스터. */
#define LSR_REG (IO_BASE + 5)   /* 라인 상태 레지스터 (읽기 전용). */

/* 인터럽트 허용 레지스터 비트. */
#define IER_RECV 0x01           /* 데이터를 받으면 인터럽트. */
#define IER_XMIT 0x02           /* 송신이 끝나면 인터럽트. */

/* 라인 제어 레지스터 비트. */
#define LCR_N81 0x03            /* 패리티 없음, 데이터 8비트, 정지 비트 1개. */
#define LCR_DLAB 0x80           /* 분주 래치 접근 비트 (DLAB). */

/* 모뎀 제어 레지스터. */
#define MCR_OUT2 0x08           /* 출력 라인 2. */

/* 라인 상태 레지스터. */
#define LSR_DR 0x01             /* 데이터 준비됨: 받은 바이트가 RBR에 있음. */
#define LSR_THRE 0x20           /* THR 비어 있음. */

/* 송신 방식. */
static enum { UNINIT, POLL, QUEUE } mode;

/* 보낼 데이터. */
static struct intq txq;

static void set_serial (int bps);
static void putc_poll (uint8_t);
static void write_ier (void);
static intr_handler_func serial_interrupt;

/* 시리얼 포트를 폴링 모드로 초기화함.
   폴링 모드는 시리얼 포트가 비기를 바쁘게 기다렸다가 씀.
   느리지만, 인터럽트가 초기화되기 전에는 이 방법밖에 없음. */
static void
init_poll (void) {
	ASSERT (mode == UNINIT);
	outb (IER_REG, 0);                    /* 모든 인터럽트를 끔. */
	outb (FCR_REG, 0);                    /* FIFO 끔. */
	set_serial (115200);                  /* 115.2 kbps, N-8-1. */
	outb (MCR_REG, MCR_OUT2);             /* 인터럽트를 켜려면 필요함. */
	intq_init (&txq);
	mode = POLL;
}

/* 시리얼 포트를 큐 기반, 인터럽트 구동 I/O 모드로 초기화함.
   인터럽트 구동 I/O에서는 시리얼 장치가 준비되기를 기다리며
   CPU 시간을 낭비하지 않음. */
void
serial_init_queue (void) {
	enum intr_level old_level;

	if (mode == UNINIT)
		init_poll ();
	ASSERT (mode == POLL);

	intr_register_ext (0x20 + 4, serial_interrupt, "serial");
	mode = QUEUE;
	old_level = intr_disable ();
	write_ier ();
	intr_set_level (old_level);
}

/* BYTE를 시리얼 포트로 보냄. */
void
serial_putc (uint8_t byte) {
	enum intr_level old_level = intr_disable ();

	if (mode != QUEUE) {
		/* 아직 인터럽트 구동 I/O 준비가 안 됐으면,
		   단순 폴링으로 바이트를 보냄. */
		if (mode == UNINIT)
			init_poll ();
		putc_poll (byte);
	} else {
		/* 그렇지 않으면 바이트를 큐에 넣고 인터럽트 허용
		   레지스터를 갱신함. */
		if (old_level == INTR_OFF && intq_full (&txq)) {
			/* 인터럽트가 꺼져 있고 송신 큐가 꽉 찬 상태임.
			   큐가 빌 때까지 기다리려면 인터럽트를 다시 켜야 하는데,
			   그건 무례한 일이라 대신 폴링으로 문자를 보냄. */
			putc_poll (intq_getc (&txq));
		}

		intq_putc (&txq, byte);
		write_ier ();
	}

	intr_set_level (old_level);
}

/* 시리얼 버퍼에 남은 내용을 폴링 모드로 모두 내보냄. */
void
serial_flush (void) {
	enum intr_level old_level = intr_disable ();
	while (!intq_empty (&txq))
		putc_poll (intq_getc (&txq));
	intr_set_level (old_level);
}

/* 입력 버퍼가 찬 정도가 바뀌었을 수 있음. 수신 인터럽트를
   막아야 할지 다시 판단함.
   입력 버퍼 함수들이 문자를 추가하거나 뺄 때 부름. */
void
serial_notify (void) {
	ASSERT (intr_get_level () == INTR_OFF);
	if (mode == QUEUE)
		write_ier ();
}

/* 시리얼 포트 속도를 초당 BPS 비트로 설정함. */
static void
set_serial (int bps) {
	int base_rate = 1843200 / 16;         /* 16550A의 기본 클럭(Hz). */
	uint16_t divisor = base_rate / bps;   /* 클럭 분주값. */

	ASSERT (bps >= 300 && bps <= 115200);

	/* DLAB 켬. */
	outb (LCR_REG, LCR_N81 | LCR_DLAB);

	/* 전송 속도 설정. */
	outb (LS_REG, divisor & 0xff);
	outb (MS_REG, divisor >> 8);

	/* DLAB 끔. */
	outb (LCR_REG, LCR_N81);
}

/* 인터럽트 허용 레지스터 갱신. */
static void
write_ier (void) {
	uint8_t ier = 0;

	ASSERT (intr_get_level () == INTR_OFF);

	/* 보낼 문자가 있으면 송신 인터럽트를 켬. */
	if (!intq_empty (&txq))
		ier |= IER_XMIT;

	/* 받은 문자를 저장할 공간이 있으면 수신 인터럽트를 켬. */
	if (!input_full ())
		ier |= IER_RECV;

	outb (IER_REG, ier);
}

/* 시리얼 포트가 준비될 때까지 폴링한 뒤 BYTE를 보냄. */
static void
putc_poll (uint8_t byte) {
	ASSERT (intr_get_level () == INTR_OFF);

	while ((inb (LSR_REG) & LSR_THRE) == 0)
		continue;
	outb (THR_REG, byte);
}

/* 시리얼 인터럽트 핸들러. */
static void
serial_interrupt (struct intr_frame *f UNUSED) {
	/* UART에 인터럽트 상태를 물어봄. 이걸 안 하면 QEMU에서
	   가끔 인터럽트를 놓침. */
	inb (IIR_REG);

	/* 받을 공간이 있고 하드웨어에 받은 바이트가 있는 동안
	   계속 바이트를 받음. */
	while (!input_full () && (inb (LSR_REG) & LSR_DR) != 0)
		input_putc (inb (RBR_REG));

	/* 보낼 바이트가 있고 하드웨어가 받을 준비가 된 동안
	   계속 바이트를 보냄. */
	while (!intq_empty (&txq) && (inb (LSR_REG) & LSR_THRE) != 0)
		outb (THR_REG, intq_getc (&txq));

	/* 큐 상태에 따라 인터럽트 허용 레지스터를 갱신함. */
	write_ier ();
}
