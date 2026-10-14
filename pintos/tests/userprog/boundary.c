/* 가상 페이지 경계를 넘어가는 데이터를 넘겨서 시스템 콜을
   깨뜨려 보는 테스트용 유틸리티 함수. */

#include <inttypes.h>
#include <round.h>
#include <string.h>
#include "tests/userprog/boundary.h"

static char dst[8192];

/* 페이지의 시작 주소를 돌려줌. 돌려준 포인터 앞뒤로 최소
   2048바이트씩 수정 가능한 공간이 있음. */
void *
get_boundary_area (void) 
{
  char *p = (char *) ROUND_UP ((uintptr_t) dst, 4096);
  if (p - dst < 2048)
    p += 4096;
  return p;
}

/* SRC를 두 페이지의 경계에 걸치도록 복사해서 돌려줌. */
char *
copy_string_across_boundary (const char *src) 
{
  char *p = get_boundary_area ();
  p -= strlen (src) < 4096 ? strlen (src) / 2 : 4096;
  strlcpy (p, src, 4096);
  return p;
}

