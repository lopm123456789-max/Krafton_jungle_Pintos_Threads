/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* 세마포어 SEMA를 VALUE로 초기화함. 세마포어는 음이 아닌 정수와,
   그 값을 다루는 원자적 연산 두 개로 이뤄짐:

   - down 또는 "P": 값이 양수가 될 때까지 기다렸다가 1 줄임.

   - up 또는 "V": 값을 1 늘림 (기다리는 스레드가 있으면 하나를
   깨움). */
void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* 세마포어의 down("P") 연산. SEMA 값이 양수가 될 때까지 기다린
   뒤 원자적으로 1 줄임.

   이 함수는 잠들 수 있으므로 인터럽트 핸들러 안에서 부르면 안 됨.
   인터럽트가 꺼진 상태로 불러도 되지만, 잠들게 되면 다음에
   스케줄된 스레드가 아마 인터럽트를 다시 켤 것임. 이것이
   sema_down 함수임. */
void
sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) {
		list_push_back (&sema->waiters, &thread_current ()->elem);
		thread_block ();
	}
	sema->value--;
	intr_set_level (old_level);
}

/* 세마포어의 down("P") 연산이지만, 세마포어가 이미 0이 아닐 때만
   함. 세마포어를 줄였으면 true, 아니면 false를 돌려줌.

   인터럽트 핸들러에서 불러도 됨. */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}

/* 세마포어의 up("V") 연산. SEMA 값을 1 늘리고, SEMA를 기다리는
   스레드가 있으면 그중 하나를 깨움.

   인터럽트 핸들러에서 불러도 됨. */
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (!list_empty (&sema->waiters))
		thread_unblock (list_entry (list_pop_front (&sema->waiters),
					struct thread, elem));
	sema->value++;
	intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* 두 스레드 사이에서 제어가 "핑퐁"처럼 오가게 하는 세마포어
   자체 테스트. 무슨 일이 일어나는지 보려면 printf() 호출을 넣어
   볼 것. */
void
sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n");
}

/* sema_self_test()가 쓰는 스레드 함수. */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}

/* LOCK을 초기화함. 락은 어느 순간이든 최대 한 스레드만 쥘 수
   있음. 우리 락은 "재귀적"이지 않음. 즉 락을 쥐고 있는 스레드가 그
   락을 다시 얻으려 하면 오류임.

   락은 초기값이 1인 세마포어의 특수한 형태임. 락과 그런 세마포어의
   차이는 두 가지임. 첫째, 세마포어는 값이 1보다 클 수 있지만, 락은
   한 번에 한 스레드만 소유할 수 있음. 둘째, 세마포어에는 소유자가
   없어서 한 스레드가 "down" 하고 다른 스레드가 "up" 해도 되지만,
   락은 같은 스레드가 획득하고 해제해야 함. 이런 제약이 부담스럽게
   느껴진다면 락 대신 세마포어를 써야 한다는 좋은 신호임. */
void
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* LOCK을 획득함. 필요하면 쓸 수 있게 될 때까지 잠듦. 현재 스레드가
   이미 이 락을 쥐고 있으면 안 됨.

   이 함수는 잠들 수 있으므로 인터럽트 핸들러 안에서 부르면 안 됨.
   인터럽트가 꺼진 상태로 불러도 되지만, 잠들어야 하면 인터럽트가
   다시 켜짐. */
void
lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (!lock_held_by_current_thread (lock));

	sema_down (&lock->semaphore);
	lock->holder = thread_current ();
}

/* LOCK 획득을 시도해서 성공하면 true, 실패하면 false를 돌려줌.
   현재 스레드가 이미 이 락을 쥐고 있으면 안 됨.

   이 함수는 잠들지 않으므로 인터럽트 핸들러 안에서 불러도 됨. */
bool
lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
	return success;
}

/* LOCK을 해제함. 현재 스레드가 소유한 락이어야 함.
   이것이 lock_release 함수임.

   인터럽트 핸들러는 락을 획득할 수 없으므로, 인터럽트 핸들러
   안에서 락을 해제하려는 것은 말이 안 됨. */
void
lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));

	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* 현재 스레드가 LOCK을 쥐고 있으면 true, 아니면 false를 돌려줌.
   (다른 스레드가 락을 쥐고 있는지 검사하는 것은 경쟁 상태가 생길
   수 있다는 점에 주의.) */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* 리스트 안의 세마포어 하나. */
struct semaphore_elem {
	struct list_elem elem;              /* 리스트 원소. */
	struct semaphore semaphore;         /* 이 세마포어. */
};

/* 조건 변수 COND를 초기화함. 조건 변수는 한쪽 코드가 조건이
   됐다고 신호를 보내고, 협력하는 다른 코드가 그 신호를 받아서
   처리할 수 있게 해줌. */
void
cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* 원자적으로 LOCK을 풀고, 다른 코드가 COND에 신호를 보낼 때까지
   기다림. COND에 신호가 오면 돌아가기 전에 LOCK을 다시 획득함.
   이 함수를 부르기 전에 LOCK을 쥐고 있어야 함.

   이 함수가 구현하는 모니터는 "Hoare" 방식이 아니라 "Mesa"
   방식임. 즉 신호를 보내는 것과 받는 것이 하나의 원자적 연산이
   아님. 그래서 보통 호출한 쪽은 기다림이 끝난 뒤 조건을 다시
   확인하고, 필요하면 다시 기다려야 함.

   조건 변수 하나는 락 하나에만 연결되지만, 락 하나에는 조건 변수가
   몇 개든 연결될 수 있음. 즉 락과 조건 변수는 일대다 관계임.

   이 함수는 잠들 수 있으므로 인터럽트 핸들러 안에서 부르면 안 됨.
   인터럽트가 꺼진 상태로 불러도 되지만, 잠들어야 하면 인터럽트가
   다시 켜짐. */
void
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
	list_push_back (&cond->waiters, &waiter.elem);
	lock_release (lock);
	sema_down (&waiter.semaphore);
	lock_acquire (lock);
}

/* (LOCK으로 보호되는) COND에서 기다리는 스레드가 있으면, 그중
   하나에게 깨어나라고 신호를 보냄.
   이 함수를 부르기 전에 LOCK을 쥐고 있어야 함.

   인터럽트 핸들러는 락을 획득할 수 없으므로, 인터럽트 핸들러
   안에서 조건 변수에 신호를 보내려는 것은 말이 안 됨. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters))
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore);
}

/* (LOCK으로 보호되는) COND에서 기다리는 스레드가 있으면 모두
   깨움. 이 함수를 부르기 전에 LOCK을 쥐고 있어야 함.

   인터럽트 핸들러는 락을 획득할 수 없으므로, 인터럽트 핸들러
   안에서 조건 변수에 신호를 보내려는 것은 말이 안 됨. */
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
