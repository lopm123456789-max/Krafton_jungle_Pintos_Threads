/* page-parallel의 자식 프로세스.
   0으로 채운 1MB를 암호화했다가 다시 복호화하고, 0으로
   돌아왔는지 확인함. */

#include <string.h>
#include "tests/arc4.h"
#include "tests/lib.h"
#include "tests/main.h"

#define SIZE (1024 * 1024)
static char buf[SIZE];

int
main (int argc, char *argv[])
{
  test_name = "child-linear";

  const char *key = argv[argc - 1];
  struct arc4 arc4;
  size_t i;

  /* 0을 암호화. */
  arc4_init (&arc4, key, strlen (key));
  arc4_crypt (&arc4, buf, SIZE);

  /* 다시 0으로 복호화. */
  arc4_init (&arc4, key, strlen (key));
  arc4_crypt (&arc4, buf, SIZE);

  /* 전부 0인지 확인. */
  for (i = 0; i < SIZE; i++)
    if (buf[i] != '\0')
      fail ("byte %zu != 0", i);

  return 0x42;
}
