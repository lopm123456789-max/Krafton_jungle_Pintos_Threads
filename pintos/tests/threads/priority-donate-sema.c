/* 낮은 우선순위 스레드 L이 락을 획득한 뒤, 세마포어를 down 하다가
   막힘. 중간 우선순위 스레드 M도 같은 세마포어에서 기다리며 막힘.
   그다음 높은 우선순위 스레드 H가 락을 얻으려 하면서 L에게
   우선순위를 기부함.

   이어서 main 스레드가 세마포어를 up 해서 L을 깨움. L은 락을 풀어서
   H를 깨움. H는 세마포어를 "up" 해서 M을 깨움. H가 종료하고, 이어서
   M, L, 마지막으로 main 스레드가 종료함.

   Godmar Back <gback@cs.vt.edu> 작성. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct lock_and_sema 
  {
    struct lock lock;
    struct semaphore sema;
  };

static thread_func l_thread_func;
static thread_func m_thread_func;
static thread_func h_thread_func;

void
test_priority_donate_sema (void) 
{
  struct lock_and_sema ls;

  /* 이 테스트는 MLFQS에서는 동작하지 않음. */
  ASSERT (!thread_mlfqs);

  /* 우리 우선순위가 기본값인지 확인. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&ls.lock);
  sema_init (&ls.sema, 0);
  thread_create ("low", PRI_DEFAULT + 1, l_thread_func, &ls);
  thread_create ("med", PRI_DEFAULT + 3, m_thread_func, &ls);
  thread_create ("high", PRI_DEFAULT + 5, h_thread_func, &ls);
  sema_up (&ls.sema);
  msg ("Main thread finished.");
}

static void
l_thread_func (void *ls_) 
{
  struct lock_and_sema *ls = ls_;

  lock_acquire (&ls->lock);
  msg ("Thread L acquired lock.");
  sema_down (&ls->sema);
  msg ("Thread L downed semaphore.");
  lock_release (&ls->lock);
  msg ("Thread L finished.");
}

static void
m_thread_func (void *ls_) 
{
  struct lock_and_sema *ls = ls_;

  sema_down (&ls->sema);
  msg ("Thread M finished.");
}

static void
h_thread_func (void *ls_) 
{
  struct lock_and_sema *ls = ls_;

  lock_acquire (&ls->lock);
  msg ("Thread H acquired lock.");

  sema_up (&ls->sema);
  lock_release (&ls->lock);
  msg ("Thread H finished.");
}
