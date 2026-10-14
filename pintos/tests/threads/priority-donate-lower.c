/* main 스레드가 락을 획득함. 그다음 그 락을 얻으려다 막히는
   더 높은 우선순위 스레드를 만들어서, 그 스레드가 main 스레드에게
   우선순위를 기부하게 함. main 스레드가 자기 우선순위를 낮추려
   해도, 기부가 풀리기 전까지는 적용되면 안 됨. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func acquire_thread_func;

void
test_priority_donate_lower (void) 
{
  struct lock lock;

  /* 이 테스트는 MLFQS에서는 동작하지 않음. */
  ASSERT (!thread_mlfqs);

  /* 우리 우선순위가 기본값인지 확인. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  lock_init (&lock);
  lock_acquire (&lock);
  thread_create ("acquire", PRI_DEFAULT + 10, acquire_thread_func, &lock);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 10, thread_get_priority ());

  msg ("Lowering base priority...");
  thread_set_priority (PRI_DEFAULT - 10);
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT + 10, thread_get_priority ());
  lock_release (&lock);
  msg ("acquire must already have finished.");
  msg ("Main thread should have priority %d.  Actual priority: %d.",
       PRI_DEFAULT - 10, thread_get_priority ());
}

static void
acquire_thread_func (void *lock_) 
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  msg ("acquire: got the lock");
  lock_release (lock);
  msg ("acquire: done");
}
