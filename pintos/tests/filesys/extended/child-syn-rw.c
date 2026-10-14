/* syn-rw용 자식 프로세스.
   부모 프로세스가 만들어서 계속 늘리고 있는 파일을 읽음. 파일 전체를
   제대로 읽을 때까지 반복함. 그사이 파일이 늘지 않았으면 0바이트를
   읽는 경우가 많음. 즉 파일이 늘어나기를 "바쁘게 기다리는" 것임.
   (0바이트를 읽을 때마다 "yield" 시스템 콜을 부르도록 하면 이
   테스트를 개선할 수 있음.) */

#include <random.h>
#include <stdlib.h>
#include <syscall.h>
#include "tests/filesys/extended/syn-rw.h"
#include "tests/lib.h"

static char buf1[BUF_SIZE];
static char buf2[BUF_SIZE];

int
main (int argc, const char *argv[]) 
{
  test_name = "child-syn-rw";

  int child_idx;
  int fd;
  size_t ofs;

  quiet = true;
  
  CHECK (argc == 2, "argc must be 2, actually %d", argc);
  child_idx = atoi (argv[1]);

  random_init (0);
  random_bytes (buf1, sizeof buf1);

  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  ofs = 0;
  while (ofs < sizeof buf2)
    {
      int bytes_read = read (fd, buf2 + ofs, sizeof buf2 - ofs);
      CHECK (bytes_read >= -1 && bytes_read <= (int) (sizeof buf2 - ofs),
             "%zu-byte read on \"%s\" returned invalid value of %d",
             sizeof buf2 - ofs, file_name, bytes_read);
      if (bytes_read > 0) 
        {
          compare_bytes (buf2 + ofs, buf1 + ofs, bytes_read, ofs, file_name);
          ofs += bytes_read;
        }
    }
  close (fd);

  return child_idx;
}
