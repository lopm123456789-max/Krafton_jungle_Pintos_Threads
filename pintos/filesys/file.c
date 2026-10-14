#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"

/* 열린 파일 하나. */
struct file {
	struct inode *inode;        /* 파일의 inode. */
	off_t pos;                  /* 현재 위치. */
	bool deny_write;            /* file_deny_write()가 불렸는지? */
};

/* 주어진 INODE로 파일을 열고 새 파일을 돌려줌. INODE의
 * 소유권은 파일이 가져감. 할당에 실패하거나 INODE가 널이면
 * 널 포인터를 돌려줌. */
struct file *
file_open (struct inode *inode) {
	struct file *file = calloc (1, sizeof *file);
	if (inode != NULL && file != NULL) {
		file->inode = inode;
		file->pos = 0;
		file->deny_write = false;
		return file;
	} else {
		inode_close (inode);
		free (file);
		return NULL;
	}
}

/* FILE과 같은 inode를 가리키는 새 파일을 열어서 돌려줌.
 * 실패하면 널 포인터를 돌려줌. */
struct file *
file_reopen (struct file *file) {
	return file_open (inode_reopen (file->inode));
}

/* 속성까지 포함해 파일 객체를 복제하고, FILE과 같은 inode를
 * 가리키는 새 파일을 돌려줌. 실패하면 널 포인터를 돌려줌. */
struct file *
file_duplicate (struct file *file) {
	struct file *nfile = file_open (inode_reopen (file->inode));
	if (nfile) {
		nfile->pos = file->pos;
		if (file->deny_write)
			file_deny_write (nfile);
	}
	return nfile;
}

/* FILE을 닫음. */
void
file_close (struct file *file) {
	if (file != NULL) {
		file_allow_write (file);
		inode_close (file->inode);
		free (file);
	}
}

/* FILE이 감싸고 있는 inode를 돌려줌. */
struct inode *
file_get_inode (struct file *file) {
	return file->inode;
}

/* FILE의 현재 위치부터 SIZE 바이트를 읽어 BUFFER에 담음.
 * 실제로 읽은 바이트 수를 돌려줌.
 * 파일 끝에 닿으면 SIZE보다 적을 수 있음.
 * FILE의 위치를 읽은 바이트 수만큼 앞으로 옮김. */
off_t
file_read (struct file *file, void *buffer, off_t size) {
	off_t bytes_read = inode_read_at (file->inode, buffer, size, file->pos);
	file->pos += bytes_read;
	return bytes_read;
}

/* 파일의 FILE_OFS 오프셋부터 SIZE 바이트를 읽어 BUFFER에 담음.
 * 실제로 읽은 바이트 수를 돌려줌.
 * 파일 끝에 닿으면 SIZE보다 적을 수 있음.
 * 파일의 현재 위치는 바뀌지 않음. */
off_t
file_read_at (struct file *file, void *buffer, off_t size, off_t file_ofs) {
	return inode_read_at (file->inode, buffer, size, file_ofs);
}

/* BUFFER의 SIZE 바이트를 FILE의 현재 위치부터 씀.
 * 실제로 쓴 바이트 수를 돌려줌.
 * 파일 끝에 닿으면 SIZE보다 적을 수 있음.
 * (원래는 이 경우 파일을 늘려야 하지만, 파일 크기 늘리기는
 * 아직 구현되지 않음.)
 * FILE의 위치를 읽은 바이트 수만큼 앞으로 옮김. */
off_t
file_write (struct file *file, const void *buffer, off_t size) {
	off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
	file->pos += bytes_written;
	return bytes_written;
}

/* BUFFER의 SIZE 바이트를 파일의 FILE_OFS 오프셋부터 씀.
 * 실제로 쓴 바이트 수를 돌려줌.
 * 파일 끝에 닿으면 SIZE보다 적을 수 있음.
 * (원래는 이 경우 파일을 늘려야 하지만, 파일 크기 늘리기는
 * 아직 구현되지 않음.)
 * 파일의 현재 위치는 바뀌지 않음. */
off_t
file_write_at (struct file *file, const void *buffer, off_t size,
		off_t file_ofs) {
	return inode_write_at (file->inode, buffer, size, file_ofs);
}

/* file_allow_write()가 불리거나 FILE이 닫힐 때까지
 * FILE이 가리키는 inode에 쓰기를 막음. */
void
file_deny_write (struct file *file) {
	ASSERT (file != NULL);
	if (!file->deny_write) {
		file->deny_write = true;
		inode_deny_write (file->inode);
	}
}

/* FILE이 가리키는 inode에 다시 쓰기를 허용함.
 * (같은 inode를 연 다른 파일이 여전히 쓰기를 막고 있을 수 있음.) */
void
file_allow_write (struct file *file) {
	ASSERT (file != NULL);
	if (file->deny_write) {
		file->deny_write = false;
		inode_allow_write (file->inode);
	}
}

/* FILE의 크기를 바이트 단위로 돌려줌. */
off_t
file_length (struct file *file) {
	ASSERT (file != NULL);
	return inode_length (file->inode);
}

/* FILE의 현재 위치를 파일 시작에서 NEW_POS 바이트 위치로 설정함. */
void
file_seek (struct file *file, off_t new_pos) {
	ASSERT (file != NULL);
	ASSERT (new_pos >= 0);
	file->pos = new_pos;
}

/* FILE의 현재 위치를 파일 시작 기준 바이트 오프셋으로 돌려줌. */
off_t
file_tell (struct file *file) {
	ASSERT (file != NULL);
	return file->pos;
}
