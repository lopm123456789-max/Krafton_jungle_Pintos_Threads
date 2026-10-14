#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"

/* 파일 시스템이 들어 있는 디스크. */
struct disk *filesys_disk;

static void do_format (void);

/* 파일 시스템 모듈을 초기화함.
 * FORMAT이 true면 파일 시스템을 다시 포맷함. */
void
filesys_init (bool format) {
	filesys_disk = disk_get (0, 1);
	if (filesys_disk == NULL)
		PANIC ("hd0:1 (hdb) not present, file system initialization failed");

	inode_init ();

#ifdef EFILESYS
	fat_init ();

	if (format)
		do_format ();

	fat_open ();
#else
	/* 기존 파일 시스템 */
	free_map_init ();

	if (format)
		do_format ();

	free_map_open ();
#endif
}

/* 파일 시스템 모듈을 종료함. 아직 안 쓴 데이터를 디스크에 씀. */
void
filesys_done (void) {
	/* 기존 파일 시스템 */
#ifdef EFILESYS
	fat_close ();
#else
	free_map_close ();
#endif
}

/* 크기가 INITIAL_SIZE인 NAME 파일을 만듦.
 * 성공하면 true, 아니면 false.
 * 같은 이름의 파일이 이미 있거나 내부 메모리 할당에 실패하면
 * 실패함. */
bool
filesys_create (const char *name, off_t initial_size) {
	disk_sector_t inode_sector = 0;
	struct dir *dir = dir_open_root ();
	bool success = (dir != NULL
			&& free_map_allocate (1, &inode_sector)
			&& inode_create (inode_sector, initial_size)
			&& dir_add (dir, name, inode_sector));
	if (!success && inode_sector != 0)
		free_map_release (inode_sector, 1);
	dir_close (dir);

	return success;
}

/* 이름이 NAME인 파일을 엶.
 * 성공하면 새 파일을, 아니면 널 포인터를 돌려줌.
 * NAME 파일이 없거나 내부 메모리 할당에 실패하면 실패함. */
struct file *
filesys_open (const char *name) {
	struct dir *dir = dir_open_root ();
	struct inode *inode = NULL;

	if (dir != NULL)
		dir_lookup (dir, name, &inode);
	dir_close (dir);

	return file_open (inode);
}

/* 이름이 NAME인 파일을 지움.
 * 성공하면 true, 실패하면 false.
 * NAME 파일이 없거나 내부 메모리 할당에 실패하면 실패함. */
bool
filesys_remove (const char *name) {
	struct dir *dir = dir_open_root ();
	bool success = dir != NULL && dir_remove (dir, name);
	dir_close (dir);

	return success;
}

/* 파일 시스템을 포맷함. */
static void
do_format (void) {
	printf ("Formatting file system...");

#ifdef EFILESYS
	/* FAT를 만들어 디스크에 저장함. */
	fat_create ();
	fat_close ();
#else
	free_map_create ();
	if (!dir_create (ROOT_DIR_SECTOR, 16))
		PANIC ("root directory creation failed");
	free_map_close ();
#endif

	printf ("done.\n");
}
