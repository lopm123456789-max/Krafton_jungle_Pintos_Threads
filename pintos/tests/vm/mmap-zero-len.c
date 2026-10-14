/* 길이 0으로 mmap 해 봄. length가 0이면 mmap은 실패해야 함.
 * 리눅스 커널 2.6.12 이전에는 길이가 0이어도 mmap이 성공했음.
 * 이 경우 매핑은 만들어지지 않고 addr을 그대로 돌려줬음.
 * 2.6.12부터는 길이 0 매핑이 실패함. Pintos의 mmap도 length가
 * 0이면 실패(즉 MAP_FAILED 반환)하기를 기대함. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

#define ACTUAL ((void *) 0x10000000)

void
test_main (void)
{
  int handle;
  void *map;

  /* mmap으로 파일에 씀. */
  CHECK (create ("sample.txt", strlen (sample)), "create \"sample.txt\"");
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  CHECK ((map = mmap (ACTUAL, 0, 0, handle, 0)) == MAP_FAILED, 
			"try to mmap zero length");
 
}
