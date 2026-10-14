#ifndef __LIB_DEBUG_H
#define __LIB_DEBUG_H

/* GCC는 함수, 함수 인자 등에 "속성(attribute)"을 붙여서
 * 성질을 표시할 수 있게 해줌.
 * 자세한 내용은 GCC 매뉴얼 참고. */
#define UNUSED __attribute__ ((unused))
#define NO_RETURN __attribute__ ((noreturn))
#define NO_INLINE __attribute__ ((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__ ((format (printf, FMT, FIRST)))

/* 소스 파일 이름, 줄 번호, 함수 이름과 사용자가 지정한
 * 메시지를 출력하고 OS를 멈춤. */
#define PANIC(...) debug_panic (__FILE__, __LINE__, __func__, __VA_ARGS__)

void debug_panic (const char *file, int line, const char *function,
		const char *message, ...) PRINTF_FORMAT (4, 5) NO_RETURN;
void debug_backtrace (void);

#endif



/* NDEBUG 설정을 바꿔가며 debug.h를 여러 번 include 할 수
 * 있도록 이 부분은 헤더 가드 밖에 둠. */
#undef ASSERT
#undef NOT_REACHED

#ifndef NDEBUG
#define ASSERT(CONDITION)                                       \
	if ((CONDITION)) { } else {                             \
		PANIC ("assertion `%s' failed.", #CONDITION);   \
	}
#define NOT_REACHED() PANIC ("executed an unreachable statement");
#else
#define ASSERT(CONDITION) ((void) 0)
#define NOT_REACHED() for (;;)
#endif /* lib/debug.h */
