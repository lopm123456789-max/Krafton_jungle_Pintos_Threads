/* 이름이 두 페이지의 경계에 걸친 스레드를 fork 함.
   올바른 동작이므로 성공해야 함. */

#include <syscall.h>
#include "tests/userprog/boundary.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  pid_t pid = fork (copy_string_across_boundary ("child-simple"));
  if (pid == 0){
    msg ("child run");
    exit(54);
  } else {
    int exit_val = wait(pid);
    CHECK (pid > 0, "fork");
    CHECK (exit_val == 54, "wait");
  }
}
