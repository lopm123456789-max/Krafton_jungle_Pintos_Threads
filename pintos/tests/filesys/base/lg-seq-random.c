/* 꽤 큰 파일을 무작위 크기 블록 단위로 순서대로 쓰고, 다시
   읽어서 제대로 써졌는지 확인함. */

#define TEST_SIZE 75678
#include "tests/filesys/base/seq-random.inc"
