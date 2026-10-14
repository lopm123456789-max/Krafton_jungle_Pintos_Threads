#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "intrinsic.h"

/* 처리한 페이지 폴트 수. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* 사용자 프로그램이 일으킬 수 있는 인터럽트의 핸들러를 등록함.

   실제 유닉스 계열 OS라면 [SV-386] 3-24, 3-25에 나온 것처럼 이런
   인터럽트 대부분을 시그널 형태로 사용자 프로세스에 넘기지만,
   우리는 시그널을 구현하지 않음. 대신 그냥 사용자 프로세스를
   종료시킴.

   페이지 폴트는 예외임. 여기서는 다른 예외와 똑같이 처리하지만,
   가상 메모리를 구현하려면 바꿔야 함.

   각 예외에 대한 설명은 [IA32-v3a] 5.15 "Exception and Interrupt
   Reference" 참고. */
void
exception_init (void) {
	/* 이 예외들은 사용자 프로그램이 INT, INT3, INTO, BOUND 명령 등으로
	   직접 일으킬 수 있음. 그래서 DPL==3으로 설정해서 사용자 프로그램이
	   이 명령들로 호출할 수 있게 함. */
	intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
	intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
	intr_register_int (5, 3, INTR_ON, kill,
			"#BR BOUND Range Exceeded Exception");

	/* 이 예외들은 DPL==0이라 사용자 프로세스가 INT 명령으로 호출할 수
	   없음. 그래도 간접적으로는 일어날 수 있음. 예를 들어 #DE는 0으로
	   나누면 생김.  */
	intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
	intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
	intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
	intr_register_int (7, 0, INTR_ON, kill,
			"#NM Device Not Available Exception");
	intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
	intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
	intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
	intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
	intr_register_int (19, 0, INTR_ON, kill,
			"#XF SIMD Floating-Point Exception");

	/* 대부분의 예외는 인터럽트를 켠 상태로 처리할 수 있음.
	   페이지 폴트는 폴트 주소가 CR2에 저장되고 그 값을 보존해야 해서
	   인터럽트를 꺼야 함. */
	intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* 예외 통계를 출력함. */
void
exception_print_stats (void) {
	printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* (아마도) 사용자 프로세스가 일으킨 예외의 핸들러. */
static void
kill (struct intr_frame *f) {
	/* 이 인터럽트는 (아마도) 사용자 프로세스가 일으킨 것임.
	   예를 들어 프로세스가 매핑되지 않은 가상 메모리에 접근하려 했을
	   수 있음 (페이지 폴트). 지금은 그냥 사용자 프로세스를 종료시킴.
	   나중에는 커널에서 페이지 폴트를 처리해야 함. 실제 유닉스 계열
	   운영체제는 대부분의 예외를 시그널로 프로세스에 다시 넘기지만,
	   우리는 시그널을 구현하지 않음. */

	/* 인터럽트 프레임의 코드 세그먼트 값을 보면 예외가 어디서
	   났는지 알 수 있음. */
	switch (f->cs) {
		case SEL_UCSEG:
			/* 사용자 코드 세그먼트이므로 예상대로 사용자 예외임.
			   사용자 프로세스를 종료시킴.  */
			printf ("%s: dying due to interrupt %#04llx (%s).\n",
					thread_name (), f->vec_no, intr_name (f->vec_no));
			intr_dump_frame (f);
			thread_exit ();

		case SEL_KCSEG:
			/* 커널 코드 세그먼트이므로 커널 버그임.
			   커널 코드는 예외를 일으키면 안 됨. (페이지 폴트가 커널 예외를
			   일으킬 수는 있지만, 여기까지 오면 안 됨.) 문제를 분명히 하려고
			   커널을 panic 시킴.  */
			intr_dump_frame (f);
			PANIC ("Kernel bug - unexpected interrupt in kernel");

		default:
			/* 다른 코드 세그먼트? 일어나면 안 되는 일임. 커널을 panic
			   시킴. */
			printf ("Interrupt %#04llx (%s) in unknown segment %04x\n",
					f->vec_no, intr_name (f->vec_no), f->cs);
			thread_exit ();
	}
}

/* 페이지 폴트 핸들러. 가상 메모리를 구현하려면 채워 넣어야 하는
   뼈대임. Project 2의 일부 풀이에서도 이 코드를 고쳐야 할 수 있음.

   들어올 때 폴트가 난 주소는 CR2(제어 레지스터 2)에 있고, 폴트
   정보는 exception.h의 PF_* 매크로에 설명된 형식으로 F의
   error_code 멤버에 들어 있음. 여기 예시 코드는 그 정보를 해석하는
   방법을 보여줌. 둘에 대한 자세한 정보는 [IA32-v3a] 5.15
   "Exception and Interrupt Reference"의 "Interrupt 14--Page Fault
   Exception (#PF)" 설명 참고. */
static void
page_fault (struct intr_frame *f) {
	bool not_present;  /* true: 존재하지 않는 페이지, false: 읽기 전용 페이지에 쓰기. */
	bool write;        /* true: 쓰기 접근, false: 읽기 접근. */
	bool user;         /* true: 사용자가 접근, false: 커널이 접근. */
	void *fault_addr;  /* 폴트 주소. */

	/* 폴트가 난 주소, 즉 폴트를 일으킨 접근의 가상 주소를 얻음.
	   코드를 가리킬 수도 있고 데이터를 가리킬 수도 있음. 폴트를 일으킨
	   명령의 주소(그건 f->rip임)와 꼭 같지는 않음. */

	fault_addr = (void *) rcr2();

	/* 인터럽트를 다시 켬 (CR2 값이 바뀌기 전에 확실히 읽으려고
	   잠깐 꺼둔 것뿐임). */
	intr_enable ();


	/* 원인 판단. */
	not_present = (f->error_code & PF_P) == 0;
	write = (f->error_code & PF_W) != 0;
	user = (f->error_code & PF_U) != 0;

#ifdef VM
	/* Project 3 이후용. */
	if (vm_try_handle_fault (f, fault_addr, user, write, not_present))
		return;
#endif

	/* 페이지 폴트 횟수 세기. */
	page_fault_cnt++;

	/* 진짜 폴트면 정보를 출력하고 종료함. */
	printf ("Page fault at %p: %s error %s page in %s context.\n",
			fault_addr,
			not_present ? "not present" : "rights violation",
			write ? "writing" : "reading",
			user ? "user" : "kernel");
	kill (f);
}

