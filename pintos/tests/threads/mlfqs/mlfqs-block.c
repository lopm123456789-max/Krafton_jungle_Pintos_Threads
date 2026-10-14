/* BLOCKED 상태인 스레드도 recent_cpu와 우선순위가 갱신되는지
   확인함.

   main 스레드는 25초 동안 잠들고, 5초 동안 바쁘게 돈 뒤 락을 풂.
   "block" 스레드는 20초 동안 바쁘게 돈 뒤 락을 얻으려 하는데, main
   스레드가 락을 풀 때까지 10초 동안 막힘. "block" 스레드가 잠든
   동안 recent_cpu가 제대로 감소했다면, main 스레드가 락을 푸는
   즉시 block 스레드가 스케줄돼야 함. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static void block_thread (void *lock_);

void
test_mlfqs_block (void) 
{
  int64_t start_time;
  struct lock lock;
  
  ASSERT (thread_mlfqs);

  msg ("Main thread acquiring lock.");
  lock_init (&lock);
  lock_acquire (&lock);
  
  msg ("Main thread creating block thread, sleeping 25 seconds...");
  thread_create ("block", PRI_DEFAULT, block_thread, &lock);
  timer_sleep (25 * TIMER_FREQ);

  msg ("Main thread spinning for 5 seconds...");
  start_time = timer_ticks ();
  while (timer_elapsed (start_time) < 5 * TIMER_FREQ)
    continue;

  msg ("Main thread releasing lock.");
  lock_release (&lock);

  msg ("Block thread should have already acquired lock.");
}

static void
block_thread (void *lock_) 
{
  struct lock *lock = lock_;
  int64_t start_time;

  msg ("Block thread spinning for 20 seconds...");
  start_time = timer_ticks ();
  while (timer_elapsed (start_time) < 20 * TIMER_FREQ)
    continue;

  msg ("Block thread acquiring lock...");
  lock_acquire (lock);

  msg ("...got it.");
}
