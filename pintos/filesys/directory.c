#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"

/* 디렉터리 하나. */
struct dir {
	struct inode *inode;                /* 실제 저장소(inode). */
	off_t pos;                          /* 현재 위치. */
};

/* 디렉터리 항목 하나. */
struct dir_entry {
	disk_sector_t inode_sector;         /* 헤더가 있는 섹터 번호. */
	char name[NAME_MAX + 1];            /* 널 문자로 끝나는 파일 이름. */
	bool in_use;                        /* 사용 중인지 비어 있는지? */
};

/* 주어진 SECTOR에 ENTRY_CNT개 항목을 담을 디렉터리를 만듦.
 * 성공하면 true, 실패하면 false. */
bool
dir_create (disk_sector_t sector, size_t entry_cnt) {
	return inode_create (sector, entry_cnt * sizeof (struct dir_entry));
}

/* 주어진 INODE의 디렉터리를 열어서 돌려줌. INODE의 소유권은
 * 디렉터리가 가져감. 실패하면 널 포인터를 돌려줌. */
struct dir *
dir_open (struct inode *inode) {
	struct dir *dir = calloc (1, sizeof *dir);
	if (inode != NULL && dir != NULL) {
		dir->inode = inode;
		dir->pos = 0;
		return dir;
	} else {
		inode_close (inode);
		free (dir);
		return NULL;
	}
}

/* 루트 디렉터리를 열어서 돌려줌.
 * 성공하면 true, 실패하면 false. */
struct dir *
dir_open_root (void) {
	return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* DIR과 같은 inode를 가리키는 새 디렉터리를 열어서 돌려줌.
 * 실패하면 널 포인터를 돌려줌. */
struct dir *
dir_reopen (struct dir *dir) {
	return dir_open (inode_reopen (dir->inode));
}

/* DIR을 없애고 관련 자원을 해제함. */
void
dir_close (struct dir *dir) {
	if (dir != NULL) {
		inode_close (dir->inode);
		free (dir);
	}
}

/* DIR이 감싸고 있는 inode를 돌려줌. */
struct inode *
dir_get_inode (struct dir *dir) {
	return dir->inode;
}

/* DIR에서 이름이 NAME인 파일을 찾음.
 * 찾으면 true를 돌려주고, EP가 널이 아니면 *EP에 디렉터리 항목을,
 * OFSP가 널이 아니면 *OFSP에 그 항목의 바이트 오프셋을 넣음.
 * 못 찾으면 false를 돌려주고 EP와 OFSP는 무시함. */
static bool
lookup (const struct dir *dir, const char *name,
		struct dir_entry *ep, off_t *ofsp) {
	struct dir_entry e;
	size_t ofs;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
			ofs += sizeof e)
		if (e.in_use && !strcmp (name, e.name)) {
			if (ep != NULL)
				*ep = e;
			if (ofsp != NULL)
				*ofsp = ofs;
			return true;
		}
	return false;
}

/* DIR에서 이름이 NAME인 파일을 찾아서
 * 있으면 true, 없으면 false를 돌려줌.
 * 성공하면 *INODE에 그 파일의 inode를, 실패하면 널 포인터를 넣음.
 * *INODE는 호출한 쪽이 닫아야 함. */
bool
dir_lookup (const struct dir *dir, const char *name,
		struct inode **inode) {
	struct dir_entry e;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	if (lookup (dir, name, &e, NULL))
		*inode = inode_open (e.inode_sector);
	else
		*inode = NULL;

	return *inode != NULL;
}

/* 이름이 NAME인 파일을 DIR에 추가함. DIR에 같은 이름의 파일이
 * 이미 있으면 안 됨. 파일의 inode는 INODE_SECTOR번 섹터에 있음.
 * 성공하면 true, 실패하면 false.
 * NAME이 잘못됐거나(예: 너무 김) 디스크나 메모리 오류가 나면
 * 실패함. */
bool
dir_add (struct dir *dir, const char *name, disk_sector_t inode_sector) {
	struct dir_entry e;
	off_t ofs;
	bool success = false;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	/* NAME이 올바른지 확인. */
	if (*name == '\0' || strlen (name) > NAME_MAX)
		return false;

	/* NAME이 이미 쓰이고 있지 않은지 확인. */
	if (lookup (dir, name, NULL, NULL))
		goto done;

	/* OFS를 빈 슬롯의 오프셋으로 설정함.
	 * 빈 슬롯이 없으면 현재 파일 끝 위치로 설정됨.

	 * inode_read_at()은 파일 끝에서만 요청보다 적게 읽음.
	 * 그렇지 않았다면, 메모리 부족 같은 일시적인 이유로 적게
	 * 읽은 게 아닌지 따로 확인해야 했을 것임. */
	for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
			ofs += sizeof e)
		if (!e.in_use)
			break;

	/* 슬롯에 씀. */
	e.in_use = true;
	strlcpy (e.name, name, sizeof e.name);
	e.inode_sector = inode_sector;
	success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

done:
	return success;
}

/* DIR에서 NAME 항목을 지움.
 * 성공하면 true, 실패하면 false.
 * 실패하는 경우는 NAME이라는 파일이 없을 때뿐임. */
bool
dir_remove (struct dir *dir, const char *name) {
	struct dir_entry e;
	struct inode *inode = NULL;
	bool success = false;
	off_t ofs;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	/* 디렉터리 항목 찾기. */
	if (!lookup (dir, name, &e, &ofs))
		goto done;

	/* inode 열기. */
	inode = inode_open (e.inode_sector);
	if (inode == NULL)
		goto done;

	/* 디렉터리 항목 지우기. */
	e.in_use = false;
	if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e)
		goto done;

	/* inode 제거. */
	inode_remove (inode);
	success = true;

done:
	inode_close (inode);
	return success;
}

/* DIR의 다음 디렉터리 항목을 읽어서 이름을 NAME에 저장함.
 * 성공하면 true, 더 읽을 항목이 없으면 false. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1]) {
	struct dir_entry e;

	while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) {
		dir->pos += sizeof e;
		if (e.in_use) {
			strlcpy (name, e.name, NAME_MAX + 1);
			return true;
		}
	}
	return false;
}
