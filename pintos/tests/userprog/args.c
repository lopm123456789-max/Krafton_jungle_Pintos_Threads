/* 커맨드라인 인자를 출력함.
   모든 args-* 테스트에서 이 프로그램을 씀. 채점은 출력을 보고
   args-* 테스트마다 다르게 함. */

#include "tests/lib.h"

int
main (int argc, char *argv[]) 
{
  int i;

  test_name = "args";

  if (((unsigned long long) argv & 7) != 0)
    msg ("argv and stack must be word-aligned, actually %p", argv);

  msg ("begin");
  msg ("argc = %d", argc);
  for (i = 0; i <= argc; i++)
    if (argv[i] != NULL)
      msg ("argv[%d] = '%s'", i, argv[i]);
    else
      msg ("argv[%d] = null", i);
  msg ("end");

  return 0;
}
