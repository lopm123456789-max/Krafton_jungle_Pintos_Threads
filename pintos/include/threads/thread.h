#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* 스레드 생애 주기의 상태들. */
enum thread_status {
	THREAD_RUNNING,     /* 실행 중인 스레드. */
	THREAD_READY,       /* 실행 중은 아니지만 실행할 준비가 된 상태. */
	THREAD_BLOCKED,     /* 어떤 사건이 일어나기를 기다리는 상태. */
	THREAD_DYING        /* 곧 없어질 상태. */
};

/* 스레드 식별자 타입.
   원하는 타입으로 다시 정의해도 됨. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* tid_t의 오류 값. */

/* 스레드 우선순위. */
#define PRI_MIN 0                       /* 가장 낮은 우선순위. */
#define PRI_DEFAULT 31                  /* 기본 우선순위. */
#define PRI_MAX 63                      /* 가장 높은 우선순위. */

/* 커널 스레드 또는 사용자 프로세스.
 *
 * 스레드 구조체는 각자 4kB 페이지 하나에 저장됨. 스레드 구조체
 * 자체는 페이지의 맨 아래(오프셋 0)에 있음. 페이지의 나머지는
 * 스레드의 커널 스택용이고, 스택은 페이지 맨 위(오프셋 4kB)에서
 * 아래로 자라 내려옴. 그림으로 보면:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * 여기서 두 가지 결론이 나옴:
 *
 *    1. 첫째, `struct thread'가 너무 커지면 안 됨. 커지면 커널
 *       스택을 둘 공간이 부족해짐. 기본 `struct thread'는 몇
 *       바이트밖에 안 됨. 1kB보다 충분히 작게 유지하는 게 좋음.
 *
 *    2. 둘째, 커널 스택이 너무 커지면 안 됨. 스택이 넘치면 스레드
 *       상태가 망가짐. 그래서 커널 함수는 큰 구조체나 배열을
 *       static이 아닌 지역 변수로 잡으면 안 됨. 대신 malloc()이나
 *       palloc_get_page()로 동적 할당해서 써야 함.
 *
 * 두 문제 중 하나가 생기면 가장 먼저 보이는 증상은 아마
 * thread_current()의 ASSERT 실패일 것임. thread_current()는 실행 중인
 * 스레드의 `struct thread'에서 `magic' 멤버가 THREAD_MAGIC인지
 * 확인함. 스택이 넘치면 보통 이 값이 바뀌어서 ASSERT가 걸림. */
/* `elem' 멤버는 두 가지 용도로 쓰임. 실행 대기열(run queue,
 * thread.c)의 원소가 될 수도 있고, 세마포어 대기 리스트(synch.c)의
 * 원소가 될 수도 있음. 이렇게 겸용할 수 있는 건 두 경우가 동시에
 * 일어나지 않기 때문임: READY 상태의 스레드만 실행 대기열에 있고,
 * BLOCKED 상태의 스레드만 세마포어 대기 리스트에 있음. */
struct thread {
	/* thread.c가 관리함. */
	tid_t tid;                          /* 스레드 식별자. */
	enum thread_status status;          /* 스레드 상태. */
	char name[16];                      /* 이름 (디버깅용). */
	int priority;                       /* 우선순위. */

	/* thread.c와 synch.c가 함께 씀. */
	struct list_elem elem;              /* 리스트 원소. */
	int64_t awake_time;

#ifdef USERPROG
	/* userprog/process.c가 관리함. */
	uint64_t *pml4;                     /* 4단계 페이지 맵(PML4) */
#endif
#ifdef VM
	/* 스레드가 가진 전체 가상 메모리를 위한 테이블. */
	struct supplemental_page_table spt;
#endif

	/* thread.c가 관리함. */
	struct intr_frame tf;               /* 문맥 전환(switching)에 쓰는 정보 */
	unsigned magic;                     /* 스택 오버플로를 감지함. */
};

/* false(기본값)면 라운드 로빈 스케줄러를 씀.
   true면 다단계 피드백 큐(MLFQ) 스케줄러를 씀.
   커널 커맨드라인 옵션 "-o mlfqs"로 정함. */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
