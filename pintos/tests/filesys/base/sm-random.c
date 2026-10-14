/* 꽤 작은 파일의 내용을 무작위 순서로 쓰고, 다시 무작위 순서로
   읽어서 제대로 써졌는지 확인함. */

#define BLOCK_SIZE 13
#define TEST_SIZE (13 * 123)
#include "tests/filesys/base/random.inc"
