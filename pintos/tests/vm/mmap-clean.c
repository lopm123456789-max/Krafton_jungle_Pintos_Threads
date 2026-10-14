/* mmap 영역은 메모리에서 실제로 데이터가 바뀐 경우에만 munmap 때
   파일에 다시 써지는지 확인함. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  static const char overwrite[] = "Now is the time for all good...";
  static char buffer[sizeof sample - 1];
  char *actual = (char *) 0x54321000;
  int handle;
  void *map;

  /* 파일을 열고 매핑한 뒤 데이터 확인. */
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
 
	CHECK ((map = mmap (actual, 4096, 0, handle, 0)) != MAP_FAILED, "mmap \"sample.txt\"");
  if (memcmp (actual, sample, strlen (sample)))
    fail ("read of mmap'd file reported bad data");

  /* 파일 수정. */
  CHECK (write (handle, overwrite, strlen (overwrite))
         == (int) strlen (overwrite),
         "write \"sample.txt\"");

  /* 매핑을 닫음. 매핑을 통해 수정하지 않았으므로 데이터가 다시
     써지면 안 됨. */
  msg ("munmap \"sample.txt\"");
  munmap (map);

  /* 파일을 다시 읽음. */
  msg ("seek \"sample.txt\"");
  seek (handle, 0);
  CHECK (read (handle, buffer, sizeof buffer) == sizeof buffer,
         "read \"sample.txt\"");

  /* 파일 덮어쓰기가 제대로 됐는지 확인. */
  if (memcmp (buffer, overwrite, strlen (overwrite))
      || memcmp (buffer + strlen (overwrite), sample + strlen (overwrite),
                 strlen (sample) - strlen (overwrite))) 
    {
      if (!memcmp (buffer, sample, strlen (sample)))
        fail ("munmap wrote back clean page");
      else
        fail ("read surprising data from file"); 
    }
  else
    msg ("file change was retained after munmap");
}
