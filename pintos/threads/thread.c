#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* struct thread의 `magic' 멤버에 넣는 임의의 값.
   스택 오버플로를 감지하는 데 씀. 자세한 내용은 thread.h 맨 위의
   큰 주석 참고. */
#define THREAD_MAGIC 0xcd6abf4b

/* 기본 스레드용 임의의 값.
   이 값은 수정하지 말 것. */
#define THREAD_BASIC 0xd42df210

/* THREAD_READY 상태인 프로세스 리스트. 즉 실행할 준비는 됐지만
   실제로 실행 중은 아닌 프로세스들임. */
static struct list ready_list;

/* idle 스레드. */
static struct thread *idle_thread;

/* 처음 스레드. init.c:main()을 실행하는 스레드임. */
static struct thread *initial_thread;

/* allocate_tid()가 쓰는 락. */
static struct lock tid_lock;

/* 스레드 소멸 요청 */
static struct list destruction_req;

/* 통계. */
static long long idle_ticks;    /* idle 상태로 보낸 타이머 tick 수. */
static long long kernel_ticks;  /* 커널 스레드에서 보낸 타이머 tick 수. */
static long long user_ticks;    /* 사용자 프로그램에서 보낸 타이머 tick 수. */

/* 스케줄링. */
#define TIME_SLICE 4            /* 스레드 하나에 주는 타이머 tick 수. */
static unsigned thread_ticks;   /* 마지막으로 양보한 뒤 지난 타이머 tick 수. */

/* false(기본값)면 라운드 로빈 스케줄러를 씀.
   true면 다단계 피드백 큐(MLFQ) 스케줄러를 씀.
   커널 커맨드라인 옵션 "-o mlfqs"로 정함. */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static void do_schedule(int status);
static void schedule (void);
static tid_t allocate_tid (void);

/* T가 올바른 스레드를 가리키는 것 같으면 true를 돌려줌. */
#define is_thread(t) ((t) != NULL && (t)->magic == THREAD_MAGIC)

/* 실행 중인 스레드를 돌려줌.
 * CPU의 스택 포인터 `rsp'를 읽고, 그 값을 페이지 시작 주소로
 * 내림함. `struct thread'는 항상 페이지 맨 앞에 있고 스택 포인터는
 * 페이지 중간 어딘가에 있으므로, 이렇게 하면 현재 스레드를 찾을
 * 수 있음. */
#define running_thread() ((struct thread *) (pg_round_down (rrsp ())))


// thread_start에서 쓸 전역 디스크립터 테이블(GDT).
// gdt는 thread_init 이후에 설정되므로,
// 먼저 임시 gdt를 설정해야 함.
static uint64_t gdt[3] = { 0, 0x00af9a000000ffff, 0x00cf92000000ffff };

/* 지금 실행 중인 코드를 스레드로 바꿔서 스레드 시스템을
   초기화함. 일반적으로는 불가능한 일이고, loader.S가 스택 바닥을
   페이지 경계에 맞춰 두었기 때문에 이 경우에만 가능함.

   실행 대기열(run queue)과 tid 락도 초기화함.

   이 함수를 부른 뒤 thread_create()로 스레드를 만들기 전에 반드시
   페이지 할당자를 초기화해야 함.

   이 함수가 끝나기 전에는 thread_current()를 부르면 안전하지 않음. */
void
thread_init (void) {
	ASSERT (intr_get_level () == INTR_OFF);

	/* 커널용 임시 gdt를 다시 불러옴.
	 * 이 gdt에는 사용자 문맥이 들어 있지 않음.
	 * 커널은 gdt_init()에서 사용자 문맥을 포함한 gdt를 다시 만듦. */
	struct desc_ptr gdt_ds = {
		.size = sizeof (gdt) - 1,
		.address = (uint64_t) gdt
	};
	lgdt (&gdt_ds);

	/* 전역 스레드 문맥 초기화 */
	lock_init (&tid_lock);
	list_init (&ready_list);
	list_init (&destruction_req);

	/* 실행 중인 스레드용 스레드 구조체를 설정함. */
	initial_thread = running_thread ();
	init_thread (initial_thread, "main", PRI_DEFAULT);
	initial_thread->status = THREAD_RUNNING;
	initial_thread->tid = allocate_tid ();
}

/* 인터럽트를 켜서 선점형 스레드 스케줄링을 시작함.
   idle 스레드도 만듦. */
void
thread_start (void) {
	/* idle 스레드 생성. */
	struct semaphore idle_started;
	sema_init (&idle_started, 0);
	thread_create ("idle", PRI_MIN, idle, &idle_started);

	/* 선점형 스레드 스케줄링 시작. */
	intr_enable ();

	/* idle 스레드가 idle_thread를 초기화할 때까지 기다림. */
	sema_down (&idle_started);
}

/* 타이머 tick마다 타이머 인터럽트 핸들러가 부름.
   그래서 이 함수는 외부 인터럽트 문맥에서 실행됨. */
void
thread_tick (void) {
	struct thread *t = thread_current ();

	/* 통계 갱신. */
	if (t == idle_thread)
		idle_ticks++;
#ifdef USERPROG
	else if (t->pml4 != NULL)
		user_ticks++;
#endif
	else
		kernel_ticks++;

	/* 선점 강제. */
	if (++thread_ticks >= TIME_SLICE)
		intr_yield_on_return ();
}

/* 스레드 통계를 출력함. */
void
thread_print_stats (void) {
	printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
			idle_ticks, kernel_ticks, user_ticks);
}

/* 이름이 NAME이고 초기 우선순위가 PRIORITY인 새 커널 스레드를
   만듦. 이 스레드는 AUX를 인자로 FUNCTION을 실행하며, ready 큐에
   추가됨. 새 스레드의 식별자를 돌려주고, 생성에 실패하면
   TID_ERROR를 돌려줌.

   thread_start()가 이미 불렸다면, 새 스레드는 thread_create()가
   반환되기 전에 스케줄될 수 있음. 심지어 thread_create()가 반환되기
   전에 종료할 수도 있음. 반대로 원래 스레드가 새 스레드가 스케줄되기
   전까지 얼마든지 오래 실행될 수도 있음. 순서를 보장해야 한다면
   세마포어나 다른 동기화 방법을 쓸 것.

   제공된 코드는 새 스레드의 `priority' 멤버를 PRIORITY로 설정하지만,
   실제 우선순위 스케줄링은 구현돼 있지 않음. 우선순위 스케줄링은
   Problem 1-3의 목표임. */
tid_t
thread_create (const char *name, int priority,
		thread_func *function, void *aux) {
	struct thread *t;
	tid_t tid;

	ASSERT (function != NULL);

	/* 스레드 할당. */
	t = palloc_get_page (PAL_ZERO);
	if (t == NULL)
		return TID_ERROR;

	/* 스레드 초기화. */
	init_thread (t, name, priority);
	tid = t->tid = allocate_tid ();

	/* 스케줄되면 kernel_thread를 부름.
	 * 참고) rdi가 첫 번째 인자, rsi가 두 번째 인자임. */
	t->tf.rip = (uintptr_t) kernel_thread;
	t->tf.R.rdi = (uint64_t) function;
	t->tf.R.rsi = (uint64_t) aux;
	t->tf.ds = SEL_KDSEG;
	t->tf.es = SEL_KDSEG;
	t->tf.ss = SEL_KDSEG;
	t->tf.cs = SEL_KCSEG;
	t->tf.eflags = FLAG_IF;

	/* 실행 대기열에 추가. */
	thread_unblock (t);

	return tid;
}

/* 현재 스레드를 재움. thread_unblock()으로 깨우기 전까지 다시
   스케줄되지 않음.

   이 함수는 인터럽트가 꺼진 상태로 불러야 함. 보통은 synch.h의
   동기화 기본 도구 중 하나를 쓰는 편이 더 나음. */
void
thread_block (void) {
	ASSERT (!intr_context ());
	ASSERT (intr_get_level () == INTR_OFF);
	thread_current ()->status = THREAD_BLOCKED;
	schedule ();
}

/* BLOCKED 상태의 스레드 T를 실행 준비(READY) 상태로 바꿈.
   T가 BLOCKED 상태가 아니면 오류임. (실행 중인 스레드를 READY로
   만들려면 thread_yield()를 쓸 것.)

   이 함수는 실행 중인 스레드를 선점하지 않음. 이건 중요할 수 있음:
   호출한 쪽이 직접 인터럽트를 껐다면, 스레드를 unblock 하는 것과
   다른 데이터를 갱신하는 것을 원자적으로 함께 처리하려고 할 수
   있기 때문임. */
void
thread_unblock (struct thread *t) {
	enum intr_level old_level;

	ASSERT (is_thread (t));

	old_level = intr_disable ();
	ASSERT (t->status == THREAD_BLOCKED);
	list_push_back (&ready_list, &t->elem);
	t->status = THREAD_READY;
	intr_set_level (old_level);
}

/* 실행 중인 스레드의 이름을 돌려줌. */
const char *
thread_name (void) {
	return thread_current ()->name;
}

/* 실행 중인 스레드를 돌려줌.
   running_thread()에 몇 가지 안전 검사를 더한 것임.
   자세한 내용은 thread.h 맨 위의 큰 주석 참고. */
struct thread *
thread_current (void) {
	struct thread *t = running_thread ();

	/* T가 정말 스레드인지 확인함.
	   이 ASSERT 중 하나라도 걸리면 스레드의 스택이 넘쳤을 수 있음.
	   스레드마다 스택이 4kB보다 작으므로, 큰 지역 배열 몇 개나 적당한
	   깊이의 재귀만으로도 스택 오버플로가 날 수 있음. */
	ASSERT (is_thread (t));
	ASSERT (t->status == THREAD_RUNNING);

	return t;
}

/* 실행 중인 스레드의 tid를 돌려줌. */
tid_t
thread_tid (void) {
	return thread_current ()->tid;
}

/* 현재 스레드를 스케줄에서 빼고 없앰. 호출한 쪽으로 절대
   돌아가지 않음. */
void
thread_exit (void) {
	ASSERT (!intr_context ());

#ifdef USERPROG
	process_exit ();
#endif

	/* 상태를 dying으로 바꾸고 다른 프로세스를 스케줄하기만 함.
	   실제 소멸은 schedule_tail() 호출 중에 일어남. */
	intr_disable ();
	do_schedule (THREAD_DYING);
	NOT_REACHED ();
}

/* CPU를 양보함. 현재 스레드는 잠들지 않으며, 스케줄러 판단에 따라
   곧바로 다시 스케줄될 수도 있음. */
void
thread_yield (void) {
	struct thread *curr = thread_current ();
	enum intr_level old_level;

	ASSERT (!intr_context ());

	old_level = intr_disable ();
	if (curr != idle_thread)
		list_push_back (&ready_list, &curr->elem);
	do_schedule (THREAD_READY);
	intr_set_level (old_level);
}

/* 현재 스레드의 우선순위를 NEW_PRIORITY로 설정함. */
void
thread_set_priority (int new_priority) {
	thread_current ()->priority = new_priority;
}

/* 현재 스레드의 우선순위를 돌려줌. */
int
thread_get_priority (void) {
	return thread_current ()->priority;
}

/* 현재 스레드의 nice 값을 NICE로 설정함. */
void
thread_set_nice (int nice UNUSED) {
	/* TODO: 여기에 구현할 것 */
}

/* 현재 스레드의 nice 값을 돌려줌. */
int
thread_get_nice (void) {
	/* TODO: 여기에 구현할 것 */
	return 0;
}

/* 시스템 load average의 100배 값을 돌려줌. */
int
thread_get_load_avg (void) {
	/* TODO: 여기에 구현할 것 */
	return 0;
}

/* 현재 스레드 recent_cpu 값의 100배를 돌려줌. */
int
thread_get_recent_cpu (void) {
	/* TODO: 여기에 구현할 것 */
	return 0;
}

/* idle 스레드. 실행할 준비가 된 다른 스레드가 없을 때 실행됨.

   idle 스레드는 처음에 thread_start()가 ready 리스트에 넣음.
   처음에 한 번 스케줄되는데, 그때 idle_thread를 초기화하고,
   thread_start()가 계속 진행할 수 있도록 받은 세마포어를 "up" 한 뒤
   곧바로 BLOCKED 상태가 됨. 그 뒤로 idle 스레드는 ready 리스트에
   절대 나타나지 않음. ready 리스트가 비어 있을 때
   next_thread_to_run()이 특별히 돌려주는 경우로만 쓰임. */
static void
idle (void *idle_started_ UNUSED) {
	struct semaphore *idle_started = idle_started_;

	idle_thread = thread_current ();
	sema_up (idle_started);

	for (;;) {
		/* 다른 스레드가 실행되게 함. */
		intr_disable ();
		thread_block ();

		/* 인터럽트를 다시 켜고 다음 인터럽트를 기다림.

		   `sti' 명령은 바로 다음 명령이 끝날 때까지 인터럽트를 막아두므로,
		   이 두 명령은 원자적으로 실행됨. 이 원자성이 중요함. 그렇지 않으면
		   인터럽트를 다시 켜는 것과 다음 인터럽트를 기다리는 것 사이에
		   인터럽트가 처리되어, 최대 클럭 tick 하나만큼의 시간을 낭비할 수
		   있음.

		   [IA32-v2a] "HLT", [IA32-v2b] "STI", [IA32-v3a] 7.11.1
		   "HLT Instruction" 참고. */
		asm volatile ("sti; hlt" : : : "memory");
	}
}

/* 커널 스레드의 기반이 되는 함수. */
static void
kernel_thread (thread_func *function, void *aux) {
	ASSERT (function != NULL);

	intr_enable ();       /* 스케줄러는 인터럽트가 꺼진 상태로 실행됨. */
	function (aux);       /* 스레드 함수 실행. */
	thread_exit ();       /* function()이 반환되면 스레드를 종료함. */
}


/* T를 이름이 NAME인 BLOCKED 스레드로 기본 초기화함. */
static void
init_thread (struct thread *t, const char *name, int priority) {
	ASSERT (t != NULL);
	ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
	ASSERT (name != NULL);

	memset (t, 0, sizeof *t);
	t->status = THREAD_BLOCKED;
	strlcpy (t->name, name, sizeof t->name);
	t->tf.rsp = (uint64_t) t + PGSIZE - sizeof (void *);
	t->priority = priority;
	t->magic = THREAD_MAGIC;
}

/* 다음에 스케줄할 스레드를 골라서 돌려줌. 실행 대기열(run
   queue)이 비어 있지 않으면 거기서 스레드를 돌려줘야 함. (실행 중인
   스레드가 계속 실행될 수 있다면 실행 대기열에 들어가 있을 것임.)
   실행 대기열이 비어 있으면 idle_thread를 돌려줌. */
static struct thread *
next_thread_to_run (void) {
	if (list_empty (&ready_list))
		return idle_thread;
	else
		return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* iretq로 스레드를 시작함 */
void
do_iret (struct intr_frame *tf) {
	__asm __volatile(
			"movq %0, %%rsp\n"
			"movq 0(%%rsp),%%r15\n"
			"movq 8(%%rsp),%%r14\n"
			"movq 16(%%rsp),%%r13\n"
			"movq 24(%%rsp),%%r12\n"
			"movq 32(%%rsp),%%r11\n"
			"movq 40(%%rsp),%%r10\n"
			"movq 48(%%rsp),%%r9\n"
			"movq 56(%%rsp),%%r8\n"
			"movq 64(%%rsp),%%rsi\n"
			"movq 72(%%rsp),%%rdi\n"
			"movq 80(%%rsp),%%rbp\n"
			"movq 88(%%rsp),%%rdx\n"
			"movq 96(%%rsp),%%rcx\n"
			"movq 104(%%rsp),%%rbx\n"
			"movq 112(%%rsp),%%rax\n"
			"addq $120,%%rsp\n"
			"movw 8(%%rsp),%%ds\n"
			"movw (%%rsp),%%es\n"
			"addq $32, %%rsp\n"
			"iretq"
			: : "g" ((uint64_t) tf) : "memory");
}

/* 새 스레드의 페이지 테이블을 활성화해서 스레드를 전환하고,
   이전 스레드가 dying 상태면 없앰.

   이 함수가 불리는 시점에는 이미 스레드 PREV에서 전환된 상태로,
   새 스레드가 실행 중이고 인터럽트는 아직 꺼져 있음.

   스레드 전환이 끝나기 전에는 printf()를 부르면 안전하지 않음.
   실제로는 printf()를 함수 끝에 넣어야 한다는 뜻임. */
static void
thread_launch (struct thread *th) {
	uint64_t tf_cur = (uint64_t) &running_thread ()->tf;
	uint64_t tf = (uint64_t) &th->tf;
	ASSERT (intr_get_level () == INTR_OFF);

	/* 핵심 전환 로직.
	 * 먼저 전체 실행 문맥을 intr_frame에 복원한 다음,
	 * do_iret을 불러서 다음 스레드로 전환함.
	 * 전환이 끝날 때까지 여기서부터는 스택을 절대 쓰면 안 됨. */
	__asm __volatile (
			/* 사용할 레지스터를 저장함. */
			"push %%rax\n"
			"push %%rbx\n"
			"push %%rcx\n"
			/* 입력을 한 번만 가져옴 */
			"movq %0, %%rax\n"
			"movq %1, %%rcx\n"
			"movq %%r15, 0(%%rax)\n"
			"movq %%r14, 8(%%rax)\n"
			"movq %%r13, 16(%%rax)\n"
			"movq %%r12, 24(%%rax)\n"
			"movq %%r11, 32(%%rax)\n"
			"movq %%r10, 40(%%rax)\n"
			"movq %%r9, 48(%%rax)\n"
			"movq %%r8, 56(%%rax)\n"
			"movq %%rsi, 64(%%rax)\n"
			"movq %%rdi, 72(%%rax)\n"
			"movq %%rbp, 80(%%rax)\n"
			"movq %%rdx, 88(%%rax)\n"
			"pop %%rbx\n"              // 저장한 rcx
			"movq %%rbx, 96(%%rax)\n"
			"pop %%rbx\n"              // 저장한 rbx
			"movq %%rbx, 104(%%rax)\n"
			"pop %%rbx\n"              // 저장한 rax
			"movq %%rbx, 112(%%rax)\n"
			"addq $120, %%rax\n"
			"movw %%es, (%%rax)\n"
			"movw %%ds, 8(%%rax)\n"
			"addq $32, %%rax\n"
			"call __next\n"         // 현재 rip를 읽음.
			"__next:\n"
			"pop %%rbx\n"
			"addq $(out_iret -  __next), %%rbx\n"
			"movq %%rbx, 0(%%rax)\n" // rip
			"movw %%cs, 8(%%rax)\n"  // cs
			"pushfq\n"
			"popq %%rbx\n"
			"mov %%rbx, 16(%%rax)\n" // eflags
			"mov %%rsp, 24(%%rax)\n" // rsp
			"movw %%ss, 32(%%rax)\n"
			"mov %%rcx, %%rdi\n"
			"call do_iret\n"
			"out_iret:\n"
			: : "g"(tf_cur), "g" (tf) : "memory"
			);
}

/* 새 프로세스를 스케줄함. 들어올 때 인터럽트가 꺼져 있어야 함.
 * 이 함수는 현재 스레드의 상태를 status로 바꾸고,
 * 실행할 다른 스레드를 찾아서 전환함.
 * schedule() 안에서는 printf()를 부르면 안전하지 않음. */
static void
do_schedule(int status) {
	ASSERT (intr_get_level () == INTR_OFF);
	ASSERT (thread_current()->status == THREAD_RUNNING);
	while (!list_empty (&destruction_req)) {
		struct thread *victim =
			list_entry (list_pop_front (&destruction_req), struct thread, elem);
		palloc_free_page(victim);
	}
	thread_current ()->status = status;
	schedule ();
}

static void
schedule (void) {
	struct thread *curr = running_thread ();
	struct thread *next = next_thread_to_run ();

	ASSERT (intr_get_level () == INTR_OFF);
	ASSERT (curr->status != THREAD_RUNNING);
	ASSERT (is_thread (next));
	/* 실행 중으로 표시. */
	next->status = THREAD_RUNNING;

	/* 새 time slice 시작. */
	thread_ticks = 0;

#ifdef USERPROG
	/* 새 주소 공간 활성화. */
	process_activate (next);
#endif

	if (curr != next) {
		/* 전환하기 전 스레드가 dying 상태면 그 struct thread를 없앰.
		   thread_exit()이 자기 발밑을 치우지 않도록 늦게 해야 함.
		   페이지가 지금 스택으로 쓰이고 있어서 여기서는 페이지 해제
		   요청만 대기열에 넣음.
		   실제 소멸 로직은 schedule() 시작 부분에서 불림. */
		if (curr && curr->status == THREAD_DYING && curr != initial_thread) {
			ASSERT (curr != next);
			list_push_back (&destruction_req, &curr->elem);
		}

		/* 스레드를 전환하기 전에, 먼저 현재 실행 중인 정보를
		 * 저장함. */
		thread_launch (next);
	}
}

/* 새 스레드에 쓸 tid를 돌려줌. */
static tid_t
allocate_tid (void) {
	static tid_t next_tid = 1;
	tid_t tid;

	lock_acquire (&tid_lock);
	tid = next_tid++;
	lock_release (&tid_lock);

	return tid;
}
