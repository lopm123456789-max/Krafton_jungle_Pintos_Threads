/* 디렉터리 /0/0/0부터 /3/2/2까지 만들고, 맨 아래 디렉터리에
   파일을 만듦. */

#include "tests/filesys/extended/mk-tree.h"
#include "tests/main.h"

void
test_main (void) 
{
  make_tree (4, 3, 3, 4);
}

