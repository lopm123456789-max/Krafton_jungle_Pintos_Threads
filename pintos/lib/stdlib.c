#include <ctype.h>
#include <debug.h>
#include <random.h>
#include <stdlib.h>
#include <stdbool.h>

/* S에 담긴 부호 있는 10진 정수 문자열을 `int'로 바꿔서
   돌려줌. */
int
atoi (const char *s) 
{
  bool negative;
  int value;

  ASSERT (s != NULL);

  /* 공백 건너뛰기. */
  while (isspace ((unsigned char) *s))
    s++;

  /* 부호 해석. */
  negative = false;
  if (*s == '+')
    s++;
  else if (*s == '-')
    {
      negative = true;
      s++;
    }

  /* 숫자 해석. 처음엔 항상 음수로 해석했다가 나중에 양수로
     바꿈. 2의 보수 체계에서는 int의 음수 범위가 양수 범위보다
     넓기 때문임. */
  for (value = 0; isdigit (*s); s++)
    value = value * 10 - (*s - '0');
  if (!negative)
    value = -value;

  return value;
}

/* AUX 함수를 불러서 A와 B를 비교함. */
static int
compare_thunk (const void *a, const void *b, void *aux) 
{
  int (**compare) (const void *, const void *) = aux;
  return (*compare) (a, b);
}

/* SIZE 바이트짜리 원소 CNT개가 들어 있는 ARRAY를 COMPARE로
   정렬함. COMPARE는 원소 A, B를 받아서 strcmp()와 같은 형식의
   결과를 돌려줘야 함. 즉 A < B면 0보다 작게, A == B면 0,
   A > B면 0보다 크게. CNT 기준으로 O(n lg n) 시간, O(1) 공간이
   걸림. */
void
qsort (void *array, size_t cnt, size_t size,
       int (*compare) (const void *, const void *)) 
{
  sort (array, cnt, size, compare_thunk, &compare);
}

/* 원소 크기가 SIZE 바이트인 ARRAY에서 1부터 시작하는
   인덱스 A_IDX와 B_IDX의 원소를 서로 바꿈. */
static void
do_swap (unsigned char *array, size_t a_idx, size_t b_idx, size_t size)
{
  unsigned char *a = array + (a_idx - 1) * size;
  unsigned char *b = array + (b_idx - 1) * size;
  size_t i;

  for (i = 0; i < size; i++)
    {
      unsigned char t = a[i];
      a[i] = b[i];
      b[i] = t;
    }
}

/* 원소 크기가 SIZE 바이트인 ARRAY에서 1부터 시작하는 인덱스
   A_IDX와 B_IDX의 원소를 비교함. 비교에는 COMPARE를 쓰고 보조
   데이터로 AUX를 넘기며, strcmp() 형식의 결과를 돌려줌. */
static int
do_compare (unsigned char *array, size_t a_idx, size_t b_idx, size_t size,
            int (*compare) (const void *, const void *, void *aux),
            void *aux) 
{
  return compare (array + (a_idx - 1) * size, array + (b_idx - 1) * size, aux);
}

/* 원소 크기가 SIZE 바이트이고 원소 CNT개인 ARRAY에서
   1부터 시작하는 인덱스 I의 원소를 아래로 "가라앉힘". 비교에는
   COMPARE를 쓰고 보조 데이터로 AUX를 넘김. */
static void
heapify (unsigned char *array, size_t i, size_t cnt, size_t size,
         int (*compare) (const void *, const void *, void *aux),
         void *aux) 
{
  for (;;) 
    {
      /* I와 그 자식들(있다면) 중 가장 큰 원소의 인덱스를
         `max'에 넣음. */
      size_t left = 2 * i;
      size_t right = 2 * i + 1;
      size_t max = i;
      if (left <= cnt && do_compare (array, left, max, size, compare, aux) > 0)
        max = left;
      if (right <= cnt
          && do_compare (array, right, max, size, compare, aux) > 0) 
        max = right;

      /* 최댓값이 이미 I에 있으면 끝. */
      if (max == i)
        break;

      /* 바꾸고 힙 아래로 계속 내려감. */
      do_swap (array, i, max, size);
      i = max;
    }
}

/* SIZE 바이트짜리 원소 CNT개가 들어 있는 ARRAY를 정렬함.
   비교에는 COMPARE를 쓰고 보조 데이터로 AUX를 넘김. COMPARE는
   원소 A, B를 받아서 strcmp()와 같은 형식의 결과를 돌려줘야 함.
   즉 A < B면 0보다 작게, A == B면 0, A > B면 0보다 크게.
   CNT 기준으로 O(n lg n) 시간, O(1) 공간이 걸림. */
void
sort (void *array, size_t cnt, size_t size,
      int (*compare) (const void *, const void *, void *aux),
      void *aux) 
{
  size_t i;

  ASSERT (array != NULL || cnt == 0);
  ASSERT (compare != NULL);
  ASSERT (size > 0);

  /* 힙을 만듦. */
  for (i = cnt / 2; i > 0; i--)
    heapify (array, i, cnt, size, compare, aux);

  /* 힙을 정렬함. */
  for (i = cnt; i > 1; i--) 
    {
      do_swap (array, 1, i, size);
      heapify (array, 1, i - 1, size, compare, aux); 
    }
}

/* SIZE 바이트짜리 원소 CNT개가 들어 있는 ARRAY에서 KEY를
   찾음. 찾으면 그 원소를, 못 찾으면 널 포인터를 돌려줌.
   일치하는 게 여러 개면 그중 아무거나 돌려줌.

   ARRAY는 COMPARE 기준으로 정렬돼 있어야 함.

   원소 비교에는 COMPARE를 씀. COMPARE는 원소 A, B를 받아서
   strcmp() 형식의 결과를 돌려줘야 함. 즉 A < B면 0보다 작게,
   A == B면 0, A > B면 0보다 크게. */
void *
bsearch (const void *key, const void *array, size_t cnt,
         size_t size, int (*compare) (const void *, const void *)) 
{
  return binary_search (key, array, cnt, size, compare_thunk, &compare);
}

/* SIZE 바이트짜리 원소 CNT개가 들어 있는 ARRAY에서 KEY를
   찾음. 찾으면 그 원소를, 못 찾으면 널 포인터를 돌려줌.
   일치하는 게 여러 개면 그중 아무거나 돌려줌.

   ARRAY는 COMPARE 기준으로 정렬돼 있어야 함.

   원소 비교에는 COMPARE를 쓰고 보조 데이터로 AUX를 넘김.
   COMPARE는 원소 A, B를 받아서 strcmp() 형식의 결과를 돌려줘야
   함. 즉 A < B면 0보다 작게, A == B면 0, A > B면 0보다 크게. */
void *
binary_search (const void *key, const void *array, size_t cnt, size_t size,
               int (*compare) (const void *, const void *, void *aux),
               void *aux) 
{
  const unsigned char *first = array;
  const unsigned char *last = array + size * cnt;

  while (first < last) 
    {
      size_t range = (last - first) / size;
      const unsigned char *middle = first + (range / 2) * size;
      int cmp = compare (key, middle, aux);

      if (cmp < 0) 
        last = middle;
      else if (cmp > 0) 
        first = middle + size;
      else
        return (void *) middle;
    }
  
  return NULL;
}

