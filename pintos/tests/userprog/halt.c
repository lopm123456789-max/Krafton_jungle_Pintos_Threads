/* halt 시스템 콜 테스트. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  halt ();
  fail ("should have halted");
}
