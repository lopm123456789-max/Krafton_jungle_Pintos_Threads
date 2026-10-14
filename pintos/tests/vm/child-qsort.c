/* 128kB 파일을 스택으로 읽어 와서, 여러 번 나눠서 정복하는 퀵
   정렬로 바이트를 "정렬"함. 정렬된 데이터는 같은 파일 제자리에
   다시 씀. */

#include <debug.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#include "tests/vm/qsort.h"

int
main (int argc UNUSED, char *argv[]) 
{
  test_name = "child-qsort";

  int handle;
  unsigned char buf[128 * 1024];
  size_t size;

  quiet = true;

  CHECK ((handle = open (argv[1])) > 1, "open \"%s\"", argv[1]);

  size = read (handle, buf, sizeof buf);
  qsort_bytes (buf, sizeof buf);
  seek (handle, 0);
  write (handle, buf, size);
  close (handle);
  
  return 72;
}
