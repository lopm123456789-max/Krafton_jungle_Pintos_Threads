/* 실행 중인 프로세스의 실행 파일은 자식 프로세스라도 수정할 수
   없어야 함을 확인함. */

#define CHILD_CNT "1"
#include "tests/userprog/rox-child.inc"
