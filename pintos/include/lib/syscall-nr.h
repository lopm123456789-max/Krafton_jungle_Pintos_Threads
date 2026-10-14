#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* 시스템 콜 번호. */
enum {
	/* Project 2 이후. */
	SYS_HALT,                   /* 운영체제를 멈춤. */
	SYS_EXIT,                   /* 이 프로세스를 종료함. */
	SYS_FORK,                   /* 현재 프로세스를 복제함. */
	SYS_EXEC,                   /* 현재 프로세스를 다른 프로그램으로 바꿈. */
	SYS_WAIT,                   /* 자식 프로세스가 끝날 때까지 기다림. */
	SYS_CREATE,                 /* 파일 생성. */
	SYS_REMOVE,                 /* 파일 삭제. */
	SYS_OPEN,                   /* 파일 열기. */
	SYS_FILESIZE,               /* 파일 크기 얻기. */
	SYS_READ,                   /* 파일에서 읽기. */
	SYS_WRITE,                  /* 파일에 쓰기. */
	SYS_SEEK,                   /* 파일 안의 위치 변경. */
	SYS_TELL,                   /* 파일 안의 현재 위치 알려주기. */
	SYS_CLOSE,                  /* 파일 닫기. */

	/* Project 3, 선택적으로 Project 4. */
	SYS_MMAP,                   /* 파일을 메모리에 매핑. */
	SYS_MUNMAP,                 /* 메모리 매핑 해제. */

	/* Project 4 전용. */
	SYS_CHDIR,                  /* 현재 디렉터리 변경. */
	SYS_MKDIR,                  /* 디렉터리 생성. */
	SYS_READDIR,                /* 디렉터리 항목 하나 읽기. */
	SYS_ISDIR,                  /* fd가 디렉터리인지 확인. */
	SYS_INUMBER,                /* fd의 inode 번호를 돌려줌. */
	SYS_SYMLINK,                /* fd의 inode 번호를 돌려줌. */

	/* Project 2 추가 과제 */
	SYS_DUP2,                   /* 파일 디스크립터 복제 */

	SYS_MOUNT,
	SYS_UMOUNT,
};

#endif /* lib/syscall-nr.h */
