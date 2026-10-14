/* 파일을 열고, 그 파일을 닫으려는 자식 프로세스를 실행함.
   (Pintos에는 파일 핸들 상속이 없으므로 실패해야 함.) 그다음 부모
   프로세스가 그 파일 핸들을 쓰려 하는데, 이건 성공해야 함. */

#include <stdio.h>
#include <syscall.h>
#include "tests/userprog/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  char child_cmd[128];
  int handle;

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");

  snprintf (child_cmd, sizeof child_cmd, "child-close %d", handle);
  
  pid_t pid;
  if (!(pid = fork("child-close"))){
    exec (child_cmd);
  }
  msg ("wait(exec()) = %d", wait (pid));

  check_file_handle (handle, "sample.txt", sample, sizeof sample - 1);
}
