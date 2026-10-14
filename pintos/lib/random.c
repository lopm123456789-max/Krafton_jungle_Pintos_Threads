#include "random.h"
#include <stdbool.h>
#include <stdint.h>
#include "debug.h"

/* RC4 기반 의사 난수 생성기(PRNG).

   RC4는 스트림 암호임. 여기서는 암호 성질 때문에 쓰는 게 아니라,
   구현이 쉽고 암호용이 아닌 목적이라면 출력이 충분히 무작위라서
   씀.

   RC4에 대한 정보는 http://en.wikipedia.org/wiki/RC4_(cipher) 참고.*/

/* RC4 상태. */
static uint8_t s[256];          /* S[]. */
static uint8_t s_i, s_j;        /* i, j. */

/* 이미 초기화됐는지? */
static bool inited;     

/* A와 B가 가리키는 바이트를 서로 바꿈. */
static inline void
swap_byte (uint8_t *a, uint8_t *b) {
	uint8_t t = *a;
	*a = *b;
	*b = t;
}

/* 주어진 SEED로 PRNG를 초기화하거나 다시 초기화함. */
void
random_init (unsigned seed) {
	uint8_t *seedp = (uint8_t *) &seed;
	int i;
	uint8_t j;

	for (i = 0; i < 256; i++) 
		s[i] = i;
	for (i = j = 0; i < 256; i++) {
		j += s[i] + seedp[i % sizeof seed];
		swap_byte (s + i, s + j);
	}

	s_i = s_j = 0;
	inited = true;
}

/* BUF에 무작위 바이트를 SIZE개 씀. */
void
random_bytes (void *buf_, size_t size) {
	uint8_t *buf;

	if (!inited)
		random_init (0);

	for (buf = buf_; size-- > 0; buf++) {
		uint8_t s_k;

		s_i++;
		s_j += s[s_i];
		swap_byte (s + s_i, s + s_j);

		s_k = s[s_i] + s[s_j];
		*buf = s[s_k];
	}
}

/* 의사 난수 unsigned long을 돌려줌.
   0 이상 n 미만의 난수가 필요하면 random_ulong() % n을 쓰면 됨. */
unsigned long
random_ulong (void) {
	unsigned long ul;
	random_bytes (&ul, sizeof ul);
	return ul;
}
