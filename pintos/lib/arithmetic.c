#include <stdint.h>

/* x86에서는 64비트 정수끼리의 나눗셈을 명령어 하나나 짧은 명령어
   묶음으로 할 수 없음. 그래서 GCC는 64비트 나눗셈과 나머지 연산을
   함수 호출로 구현함. 이 함수들은 보통 libgcc에서 가져오며, GCC가
   링크할 때 자동으로 포함함.

   그런데 어떤 x86-64 머신에는 32비트 x86 코드를 만들 수 있는
   컴파일러와 도구는 있지만, libgcc를 포함한 필요한 라이브러리가
   없음. 그래서 64비트 나눗셈 루틴을 직접 구현하면 이런 머신에서도
   Pintos가 동작하게 할 수 있음. Pintos가 libgcc에서 필요로 하는
   루틴은 이것뿐임.

   완결성도 이 루틴을 넣는 또 다른 이유임. Pintos가 외부에 전혀
   의존하지 않으면 그만큼 덜 신비롭게 느껴짐. */

/* x86 DIVL 명령으로 64비트 N을 32비트 D로 나눠 32비트 몫을
   구함. 몫을 돌려줌.
   몫이 32비트에 안 들어가면 나눗셈 오류(#DE) 트랩이 걸림. */
static inline uint32_t
divl (uint64_t n, uint32_t d) {
	uint32_t n1 = n >> 32;
	uint32_t n0 = n;
	uint32_t q, r;

	asm ("divl %4"
			: "=d" (r), "=a" (q)
			: "0" (n1), "1" (n0), "rm" (d));

	return q;
}

/* X의 앞쪽(최상위)부터 연속된 0 비트 개수를 돌려줌.
   X는 0이 아니어야 함. */
static int
nlz (uint32_t x) {
	/* 이 방법은 어디서나 쓸 수 있지만, 특정 시스템에서는 더 좋은
	   방법이 있음. 충분히 최신 GCC라면 __builtin_clz()를 써서
	   GCC가 아는 방법을 활용할 수 있음. 아니면 x86 BSR 명령을
	   직접 써도 됨. */
	int n = 0;
	if (x <= 0x0000FFFF) {
		n += 16;
		x <<= 16;
	}
	if (x <= 0x00FFFFFF) {
		n += 8;
		x <<= 8;
	}
	if (x <= 0x0FFFFFFF) {
		n += 4;
		x <<= 4;
	}
	if (x <= 0x3FFFFFFF) {
		n += 2;
		x <<= 2;
	}
	if (x <= 0x7FFFFFFF)
		n++;
	return n;
}

/* 부호 없는 64비트 N을 부호 없는 64비트 D로 나눈 몫을 돌려줌. */
static uint64_t
udiv64 (uint64_t n, uint64_t d) {
	if ((d >> 32) == 0) {
		/* 정확성 증명:

		   n, d, b, n1, n0은 이 함수에서 정의한 대로임.
		   [x]는 x의 "내림(floor)"을 뜻함. T = b[n1/d]라 하고,
		   d는 0이 아니라고 가정함. 그러면:
		   [n/d] = [n/d] - T + T
		   = [n/d - T] + T                         아래 (1)에 의해
		   = [(b*n1 + n0)/d - T] + T               n의 정의에 의해
		   = [(b*n1 + n0)/d - dT/d] + T
		   = [(b(n1 - d[n1/d]) + n0)/d] + T
		   = [(b[n1 % d] + n0)/d] + T,             %의 정의에 의해
		   이것이 아래에서 계산하는 식임.

		   (1) 임의의 실수 x와 정수 i에 대해 [x] + i = [x + i]임.

		   divl()이 트랩을 일으키지 않으려면 [(b[n1 % d] + n0)/d]가
		   b보다 작아야 함. [n1 % d]와 n0가 각각 최댓값 d - 1과
		   b - 1을 가진다고 하면:
		   [(b(d - 1) + (b - 1))/d] < b
		   <=> [(bd - 1)/d] < b
		   <=> [b - 1/d] < b
		   이건 항상 참임.

		   따라서 이 코드는 정확하고 트랩도 일어나지 않음. */
		uint64_t b = 1ULL << 32;
		uint32_t n1 = n >> 32;
		uint32_t n0 = n;
		uint32_t d0 = d;

		return divl (b * (n1 % d0) + n0, d0) + b * (n1 / d0);
	} else {
		/* http://www.hackersdelight.org/revisions.pdf 의 알고리즘과
		 * 증명을 바탕으로 함. */
		if (n < d)
			return 0;
		else {
			uint32_t d1 = d >> 32;
			int s = nlz (d1);
			uint64_t q = divl (n >> 1, (d << s) >> 32) >> (31 - s);
			return n - (q - 1) * d < d ? q - 1 : q;
		}
	}
}

/* 부호 없는 64비트 N을 부호 없는 64비트 D로 나눈 나머지를
   돌려줌. */
static uint32_t
umod64 (uint64_t n, uint64_t d) {
	return n - d * udiv64 (n, d);
}

/* 부호 있는 64비트 N을 부호 있는 64비트 D로 나눈 몫을 돌려줌. */
static int64_t
sdiv64 (int64_t n, int64_t d) {
	uint64_t n_abs = n >= 0 ? (uint64_t) n : -(uint64_t) n;
	uint64_t d_abs = d >= 0 ? (uint64_t) d : -(uint64_t) d;
	uint64_t q_abs = udiv64 (n_abs, d_abs);
	return (n < 0) == (d < 0) ? (int64_t) q_abs : -(int64_t) q_abs;
}

/* 부호 있는 64비트 N을 부호 있는 64비트 D로 나눈 나머지를
   돌려줌. */
static int32_t
smod64 (int64_t n, int64_t d) {
	return n - d * sdiv64 (n, d);
}

/* GCC가 부르는 루틴들. */

long long __divdi3 (long long n, long long d);
long long __moddi3 (long long n, long long d);
unsigned long long __udivdi3 (unsigned long long n, unsigned long long d);
unsigned long long __umoddi3 (unsigned long long n, unsigned long long d);

/* 부호 있는 64비트 나눗셈. */
long long
__divdi3 (long long n, long long d) {
	return sdiv64 (n, d);
}

/* 부호 있는 64비트 나머지. */
long long
__moddi3 (long long n, long long d) {
	return smod64 (n, d);
}

/* 부호 없는 64비트 나눗셈. */
unsigned long long
__udivdi3 (unsigned long long n, unsigned long long d) {
	return udiv64 (n, d);
}

/* 부호 없는 64비트 나머지. */
unsigned long long
__umoddi3 (unsigned long long n, unsigned long long d) {
	return umod64 (n, d);
}
