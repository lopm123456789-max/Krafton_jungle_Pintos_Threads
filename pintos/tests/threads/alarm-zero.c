/* timer_sleep(0) 테스트. 바로 반환해야 함. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

void
test_alarm_zero (void) 
{
  timer_sleep (0);
  pass ();
}
