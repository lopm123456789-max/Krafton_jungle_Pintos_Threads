/* 약 1MB의 무작위 데이터를 만들어 16조각으로 나눔. 별도의 자식
   프로세스가 각 조각을 정렬하며, 자식 프로세스들은 동시에 실행됨.
   그다음 조각들을 합치고 결과가 맞는지 확인함. */

#include "tests/vm/parallel-merge.h"
#include <stdio.h>
#include <syscall.h>
#include "tests/arc4.h"
#include "tests/lib.h"
#include "tests/main.h"

#define CHUNK_SIZE (128 * 1024)
#define CHUNK_CNT 8                             /* 조각 수. */
#define DATA_SIZE (CHUNK_CNT * CHUNK_SIZE)      /* 버퍼 크기. */

unsigned char buf1[DATA_SIZE], buf2[DATA_SIZE];
size_t histogram[256];

/* buf1을 무작위 데이터로 초기화하고, 그 안에서 값마다 몇 번
   나오는지 셈. */
static void
init (void)
{
  struct arc4 arc4;
  size_t i;

  msg ("init");

  arc4_init (&arc4, "foobar", 6);
  arc4_crypt (&arc4, buf1, sizeof buf1);
  for (i = 0; i < sizeof buf1; i++)
    histogram[buf1[i]]++;
}

/* SUBPROCESS로 buf1의 각 조각을 정렬함.
   SUBPROCESS는 EXIT_STATUS를 돌려줘야 함. */
static void
sort_chunks (const char *subprocess, int exit_status)
{
  pid_t children[CHUNK_CNT];
  size_t i;

  for (i = 0; i < CHUNK_CNT; i++)
    {
      char fn[128];
      char cmd[128];
      int handle;

      msg ("sort chunk %zu", i);

      /* 이 조각을 파일에 씀. */
      snprintf (fn, sizeof fn, "buf%zu", i);
      create (fn, CHUNK_SIZE);
      quiet = true;
      CHECK ((handle = open (fn)) > 1, "open \"%s\"", fn);
      write (handle, buf1 + CHUNK_SIZE * i, CHUNK_SIZE);
      close (handle);

      /* 자식 프로세스로 정렬. */
      snprintf (cmd, sizeof cmd, "%s %s", subprocess, fn);
      children[i] = fork (subprocess);
      if (children[i] == 0)
        CHECK ((children[i] = exec (cmd)) != -1, "exec \"%s\"", cmd);
      quiet = false;
    }

  for (i = 0; i < CHUNK_CNT; i++)
    {
      char fn[128];
      int handle;

      CHECK (wait (children[i]) == exit_status, "wait for child %zu", i);

      /* 파일에서 조각을 다시 읽음. */
      quiet = true;
      snprintf (fn, sizeof fn, "buf%zu", i);
      CHECK ((handle = open (fn)) > 1, "open \"%s\"", fn);
      read (handle, buf1 + CHUNK_SIZE * i, CHUNK_SIZE);
      close (handle);
      quiet = false;
    }
}

/* buf1의 정렬된 조각들을 합쳐서 완전히 정렬된 buf2를 만듦. */
static void
merge (void)
{
  unsigned char *mp[CHUNK_CNT];
  size_t mp_left;
  unsigned char *op;
  size_t i;

  msg ("merge");

  /* 병합 포인터 초기화. */
  mp_left = CHUNK_CNT;
  for (i = 0; i < CHUNK_CNT; i++)
    mp[i] = buf1 + CHUNK_SIZE * i;

  /* 병합. */
  op = buf2;
  while (mp_left > 0)
    {
      /* 가장 작은 값 찾기. */
      size_t min = 0;
      for (i = 1; i < mp_left; i++)
        if (*mp[i] < *mp[min])
          min = i;

      /* buf2에 값 추가. */
      *op++ = *mp[min];

      /* 병합 포인터를 앞으로 옮김.
         조각이 비면 집합에서 지움. */
      if ((++mp[min] - buf1) % CHUNK_SIZE == 0)
        mp[min] = mp[--mp_left];
    }
}

static void
verify (void)
{
  size_t buf_idx;
  size_t hist_idx;

  msg ("verify");

  buf_idx = 0;
  for (hist_idx = 0; hist_idx < sizeof histogram / sizeof *histogram;
       hist_idx++)
    {
      while (histogram[hist_idx]-- > 0)
        {
          if (buf2[buf_idx] != hist_idx)
            fail ("bad value %d in byte %zu", buf2[buf_idx], buf_idx);
          buf_idx++;
        }
    }

  msg ("success, buf_idx=%'zu", buf_idx);
}

void
parallel_merge (const char *child_name, int exit_status)
{
  init ();
  sort_chunks (child_name, exit_status);
  merge ();
  verify ();
}
