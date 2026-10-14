#ifndef __LIB_STDARG_H
#define __LIB_STDARG_H

/* GCC가 <stdarg.h> 기능을 내장 함수로 제공하므로
 * 그걸 그대로 쓰면 됨. */

typedef __builtin_va_list va_list;

#define va_start(LIST, ARG)	__builtin_va_start (LIST, ARG)
#define va_end(LIST)            __builtin_va_end (LIST)
#define va_arg(LIST, TYPE)	__builtin_va_arg (LIST, TYPE)
#define va_copy(DST, SRC)	__builtin_va_copy (DST, SRC)

#endif /* lib/stdarg.h */
