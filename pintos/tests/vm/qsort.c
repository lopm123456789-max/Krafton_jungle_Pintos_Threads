#include "tests/vm/qsort.h"
#include <stdbool.h>
#include <debug.h>
#include <random.h>

/* BUF의 SIZE 바이트 중에서 퀵 정렬의 피벗을 고름. */
static unsigned char
pick_pivot (unsigned char *buf, size_t size) 
{
  ASSERT (size >= 1);
  return buf[random_ulong () % size];
}

/* ARRAY의 SIZE 바이트가 앞쪽 LEFT_SIZE개는 모두 PIVOT보다 작고,
   나머지 SIZE - LEFT_SIZE개는 모두 PIVOT 이상으로 나뉘어 있는지
   확인함. */
static bool
is_partitioned (const unsigned char *array, size_t size,
                unsigned char pivot, size_t left_size) 
{
  size_t i;
  
  for (i = 0; i < left_size; i++)
    if (array[i] >= pivot)
      return false;

  for (; i < size; i++)
    if (array[i] < pivot)
      return false;

  return true;
}

/* *A와 *B의 바이트를 서로 바꿈. */
static void
swap (unsigned char *a, unsigned char *b) 
{
  unsigned char t = *a;
  *a = *b;
  *b = t;
}

/* ARRAY를 제자리에서 나눔. 앞쪽은 모두 PIVOT보다 작은 바이트,
   뒤쪽은 모두 PIVOT 이상인 바이트가 되게 함. 앞쪽 구간의 길이를
   돌려줌. */
static size_t
partition (unsigned char *array, size_t size, int pivot) 
{
  size_t left_size = size;
  unsigned char *first = array;
  unsigned char *last = first + left_size;

  for (;;)
    {
      /* FIRST를 앞으로 옮겨서 PIVOT보다 큰 첫 원소를 가리키게 함. */
      for (;;)
        {
          if (first == last)
            {
              ASSERT (is_partitioned (array, size, pivot, left_size));
              return left_size;
            }
          else if (*first >= pivot)
            break;

          first++;
        }
      left_size--;

      /* LAST를 뒤로 옮겨서 PIVOT보다 크지 않은 마지막 원소를
         가리키게 함. */
      for (;;)
        {
          last--;

          if (first == last)
            {
              ASSERT (is_partitioned (array, size, pivot, left_size));
              return left_size;
            }
          else if (*last < pivot)
            break;
          else
            left_size--;
        }

      /* FIRST와 LAST를 바꿔서, PREDICATE를 통과하는 앞쪽 구간과
         통과하지 못하는 뒤쪽 구간을 각각 늘림. */
      swap (first, last);
      first++;
    }
}

/* BUF의 SIZE 바이트가 감소하지 않는 순서면 true, 아니면 false. */
static bool
is_sorted (const unsigned char *buf, size_t size) 
{
  size_t i;

  for (i = 1; i < size; i++)
    if (buf[i - 1] > buf[i])
      return false;

  return true;
}

/* BUF의 SIZE 바이트를 퀵 정렬 알고리즘으로 감소하지 않는 순서로
   정렬함. */
void
qsort_bytes (unsigned char *buf, size_t size) 
{
  if (!is_sorted (buf, size)) 
    {
      int pivot = pick_pivot (buf, size);

      unsigned char *left_half = buf;
      size_t left_size = partition (buf, size, pivot);
      unsigned char *right_half = left_half + left_size;
      size_t right_size = size - left_size;
  
      if (left_size <= right_size) 
        {
          qsort_bytes (left_half, left_size);
          qsort_bytes (right_half, right_size); 
        }
      else
        {
          qsort_bytes (right_half, right_size); 
          qsort_bytes (left_half, left_size);
        }
    } 
}
