#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <syscall-nr.h>

/* 표준 vprintf() 함수.
   printf()와 같지만 va_list를 받음. */
int
vprintf (const char *format, va_list args) {
	return vhprintf (STDOUT_FILENO, format, args);
}

/* printf()와 같지만, 출력을 주어진 HANDLE에 씀. */
int
hprintf (int handle, const char *format, ...) {
	va_list args;
	int retval;

	va_start (args, format);
	retval = vhprintf (handle, format, args);
	va_end (args);

	return retval;
}

/* 문자열 S를 콘솔에 쓰고 줄바꿈 문자를 덧붙임. */
int
puts (const char *s) {
	write (STDOUT_FILENO, s, strlen (s));
	putchar ('\n');

	return 0;
}

/* C를 콘솔에 씀. */
int
putchar (int c) {
	char c2 = c;
	write (STDOUT_FILENO, &c2, 1);
	return c;
}

/* vhprintf_helper()에 넘기는 보조 데이터. */
struct vhprintf_aux {
	char buf[64];       /* 문자 버퍼. */
	char *p;            /* 버퍼 안의 현재 위치. */
	int char_cnt;       /* 지금까지 쓴 전체 문자 수. */
	int handle;         /* 출력 파일 핸들. */
};

static void add_char (char, void *);
static void flush (struct vhprintf_aux *);

/* printf() 서식 문자열 FORMAT에 ARGS의 인자를 적용해서 주어진
   HANDLE에 출력함. */
int
vhprintf (int handle, const char *format, va_list args) {
	struct vhprintf_aux aux;
	aux.p = aux.buf;
	aux.char_cnt = 0;
	aux.handle = handle;
	__vprintf (format, args, add_char, &aux);
	flush (&aux);
	return aux.char_cnt;
}

/* AUX의 버퍼에 C를 추가함. 버퍼가 차면 내보냄. */
static void
add_char (char c, void *aux_) {
	struct vhprintf_aux *aux = aux_;
	*aux->p++ = c;
	if (aux->p >= aux->buf + sizeof aux->buf)
		flush (aux);
	aux->char_cnt++;
}

/* AUX의 버퍼를 내보냄. */
static void
flush (struct vhprintf_aux *aux) {
	if (aux->p > aux->buf)
		write (aux->handle, aux->buf, aux->p - aux->buf);
	aux->p = aux->buf;
}
