/* main 스레드는 자기 우선순위를 PRI_MIN으로 정하고, 우선순위가
   PRI_MIN + 3, 6, 9, 12, ...인 스레드 7개(thread 1..7)를 만듦.
   main 스레드는 락 8개(lock 0..7)를 초기화하고 lock 0을 획득함.

   thread[i]는 시작하면 먼저 lock[i]를 획득함 (i == 7은 제외).
   그다음 thread[i]는 lock[i-1]을 획득하려 함. lock[i-1]은
   thread[i-1]이 쥐고 있고, lock[0]만 main 스레드가 쥐고 있음.
   락이 잡혀 있으므로 thread[i]는 thread[i-1]에게 우선순위를 기부하고,
   thread[i-1]은 thread[i-2]에게 기부하는 식으로 이어져서 결국 main
   스레드가 기부를 받음.

   threads[1..7]이 생성되어 locks[0..7]에서 막힌 뒤, main 스레드가
   lock[0]을 풀면 thread[1]이 깨어나고 main은 thread[1]에게 선점당함.
   thread[1]은 lock[0] 획득을 마친 뒤 lock[0]을 풀고, lock[1]을
   풀어서 thread[2]를 깨움. 이런 식으로 이어짐.
   마지막으로 thread[7]이 lock[7]을 획득했다 풀고 종료하면,
   thread[6], thread[5] 순으로 실행되고 종료하며, 마지막에 main
   스레드가 종료함.

   추가로 우선순위 p = PRI_MIN + 2, 5, 8, 11, ...인 끼어들기
   (interloper) 스레드들도 만듦. 이 스레드들은 우선순위 p + 1인
   스레드가 끝나기 전까지는 실행되면 안 됨.

   Godmar Back <gback@cs.vt.edu> 작성 */ 

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define NESTING_DEPTH 8

struct lock_pair
  {
    struct lock *second;
    struct lock *first;
  };

static thread_func donor_thread_func;
static thread_func interloper_thread_func;

void
test_priority_donate_chain (void) 
{
  int i;  
  struct lock locks[NESTING_DEPTH - 1];
  struct lock_pair lock_pairs[NESTING_DEPTH];

  /* 이 테스트는 MLFQS에서는 동작하지 않음. */
  ASSERT (!thread_mlfqs);

  thread_set_priority (PRI_MIN);

  for (i = 0; i < NESTING_DEPTH - 1; i++)
    lock_init (&locks[i]);

  lock_acquire (&locks[0]);
  msg ("%s got lock.", thread_name ());

  for (i = 1; i < NESTING_DEPTH; i++)
    {
      char name[16];
      int thread_priority;

      snprintf (name, sizeof name, "thread %d", i);
      thread_priority = PRI_MIN + i * 3;
      lock_pairs[i].first = i < NESTING_DEPTH - 1 ? locks + i: NULL;
      lock_pairs[i].second = locks + i - 1;

      thread_create (name, thread_priority, donor_thread_func, lock_pairs + i);
      msg ("%s should have priority %d.  Actual priority: %d.",
          thread_name (), thread_priority, thread_get_priority ());

      snprintf (name, sizeof name, "interloper %d", i);
      thread_create (name, thread_priority - 1, interloper_thread_func, NULL);
    }

  lock_release (&locks[0]);
  msg ("%s finishing with priority %d.", thread_name (),
                                         thread_get_priority ());
}

static void
donor_thread_func (void *locks_) 
{
  struct lock_pair *locks = locks_;

  if (locks->first)
    lock_acquire (locks->first);

  lock_acquire (locks->second);
  msg ("%s got lock", thread_name ());

  lock_release (locks->second);
  msg ("%s should have priority %d. Actual priority: %d", 
        thread_name (), (NESTING_DEPTH - 1) * 3,
        thread_get_priority ());

  if (locks->first)
    lock_release (locks->first);

  msg ("%s finishing with priority %d.", thread_name (),
                                         thread_get_priority ());
}

static void
interloper_thread_func (void *arg_ UNUSED)
{
  msg ("%s finished.", thread_name ());
}

// vim: sw=2
