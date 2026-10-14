#include "userprog/tss.h"
#include <debug.h>
#include <stddef.h>
#include "userprog/gdt.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "intrinsic.h"

/* 태스크 상태 세그먼트(TSS).
 *
 *  TSS는 x86-64 전용 구조체로, 프로세서에 내장된 멀티태스킹 지원
 *  방식인 "태스크"를 정의하는 데 씀. 하지만 이식성, 속도, 유연성
 *  등 여러 이유로 대부분의 x86-64 OS는 TSS를 거의 완전히 무시함.
 *  우리도 마찬가지임.
 *
 *  아쉽게도 TSS로만 할 수 있는 일이 하나 있음: 사용자 모드에서
 *  일어난 인터럽트를 위한 스택 전환임. 사용자 모드(링 3)에서
 *  인터럽트가 일어나면, 프로세서는 현재 TSS의 rsp0 멤버를 보고
 *  인터럽트 처리에 쓸 스택을 정함. 그래서 TSS를 만들고 최소한 이
 *  필드들은 초기화해야 하며, 이 파일이 바로 그 일을 함.
 *
 *  인터럽트 게이트나 트랩 게이트로 인터럽트를 처리할 때(우리가
 *  처리하는 모든 인터럽트가 해당됨) x86-64 프로세서는 이렇게
 *  동작함:
 *
 *    - 인터럽트에 걸린 코드가 인터럽트 핸들러와 같은 링에 있으면
 *      스택 전환이 일어나지 않음. 커널에서 실행 중일 때 일어난
 *      인터럽트가 이 경우임. 이때는 TSS 내용이 상관없음.
 *
 *    - 인터럽트에 걸린 코드가 핸들러와 다른 링에 있으면, 프로세서는
 *      새 링용으로 TSS에 지정된 스택으로 전환함. 사용자 공간에 있을
 *      때 일어난 인터럽트가 이 경우임. 손상을 막으려면 이미 쓰이고
 *      있지 않은 스택으로 전환하는 게 중요함. 사용자 공간에서 실행
 *      중이었으므로 현재 프로세스의 커널 스택은 쓰이고 있지 않음을
 *      알고 있고, 그래서 항상 그 스택을 쓸 수 있음. 그래서
 *      스케줄러가 스레드를 전환할 때 TSS의 스택 포인터도 새 스레드의
 *      커널 스택을 가리키도록 바꿈.
 *      (그 호출은 thread.c의 schedule에 있음.) */

/* 커널 TSS. */
struct task_state *tss;

/* 커널 TSS를 초기화함. */
void
tss_init (void) {
	/* 우리 TSS는 콜 게이트나 태스크 게이트에서 쓰이지 않으므로,
	 * 실제로 참조되는 필드는 몇 개뿐이고 그것만 초기화함. */
	tss = palloc_get_page (PAL_ASSERT | PAL_ZERO);
	tss_update (thread_current ());
}

/* 커널 TSS를 돌려줌. */
struct task_state *
tss_get (void) {
	ASSERT (tss != NULL);
	return tss;
}

/* TSS의 링 0 스택 포인터가 스레드 스택의 끝을 가리키도록
 * 설정함. */
void
tss_update (struct thread *next) {
	ASSERT (tss != NULL);
	tss->rsp0 = (uint64_t) next + PGSIZE;
}
