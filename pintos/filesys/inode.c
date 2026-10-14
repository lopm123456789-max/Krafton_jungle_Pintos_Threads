#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* inode를 식별하는 값. */
#define INODE_MAGIC 0x494e4f44

/* 디스크에 저장되는 inode.
 * 크기가 정확히 DISK_SECTOR_SIZE 바이트여야 함. */
struct inode_disk {
	disk_sector_t start;                /* 첫 데이터 섹터. */
	off_t length;                       /* 파일 크기(바이트). */
	unsigned magic;                     /* 매직 넘버. */
	uint32_t unused[125];               /* 사용 안 함. */
};

/* SIZE 바이트짜리 inode에 할당할 섹터 수를 돌려줌. */
static inline size_t
bytes_to_sectors (off_t size) {
	return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* 메모리에 올라온 inode. */
struct inode {
	struct list_elem elem;              /* inode 리스트의 원소. */
	disk_sector_t sector;               /* 디스크상 위치의 섹터 번호. */
	int open_cnt;                       /* 이 inode를 연 횟수. */
	bool removed;                       /* 삭제됐으면 true, 아니면 false. */
	int deny_write_cnt;                 /* 0: 쓰기 가능, 0보다 크면: 쓰기 금지. */
	struct inode_disk data;             /* inode 내용. */
};

/* INODE 안에서 바이트 오프셋 POS가 들어 있는 디스크 섹터를
 * 돌려줌.
 * INODE에 POS 위치의 데이터가 없으면 -1을 돌려줌. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos) {
	ASSERT (inode != NULL);
	if (pos < inode->data.length)
		return inode->data.start + pos / DISK_SECTOR_SIZE;
	else
		return -1;
}

/* 열린 inode 목록. 같은 inode를 두 번 열면 같은
 * `struct inode'를 돌려주기 위해 씀. */
static struct list open_inodes;

/* inode 모듈을 초기화함. */
void
inode_init (void) {
	list_init (&open_inodes);
}

/* LENGTH 바이트 데이터를 가진 inode를 초기화하고, 파일 시스템
 * 디스크의 SECTOR번 섹터에 새 inode를 씀.
 * 성공하면 true를 돌려줌.
 * 메모리나 디스크 할당에 실패하면 false를 돌려줌. */
bool
inode_create (disk_sector_t sector, off_t length) {
	struct inode_disk *disk_inode = NULL;
	bool success = false;

	ASSERT (length >= 0);

	/* 이 ASSERT가 실패하면 inode 구조체 크기가 정확히 섹터
	 * 하나가 아니라는 뜻이니 고쳐야 함. */
	ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

	disk_inode = calloc (1, sizeof *disk_inode);
	if (disk_inode != NULL) {
		size_t sectors = bytes_to_sectors (length);
		disk_inode->length = length;
		disk_inode->magic = INODE_MAGIC;
		if (free_map_allocate (sectors, &disk_inode->start)) {
			disk_write (filesys_disk, sector, disk_inode);
			if (sectors > 0) {
				static char zeros[DISK_SECTOR_SIZE];
				size_t i;

				for (i = 0; i < sectors; i++) 
					disk_write (filesys_disk, disk_inode->start + i, zeros); 
			}
			success = true; 
		} 
		free (disk_inode);
	}
	return success;
}

/* SECTOR에서 inode를 읽어서 그걸 담은 `struct inode'를 돌려줌.
 * 메모리 할당에 실패하면 널 포인터를 돌려줌. */
struct inode *
inode_open (disk_sector_t sector) {
	struct list_elem *e;
	struct inode *inode;

	/* 이 inode가 이미 열려 있는지 확인. */
	for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
			e = list_next (e)) {
		inode = list_entry (e, struct inode, elem);
		if (inode->sector == sector) {
			inode_reopen (inode);
			return inode; 
		}
	}

	/* 메모리 할당. */
	inode = malloc (sizeof *inode);
	if (inode == NULL)
		return NULL;

	/* 초기화. */
	list_push_front (&open_inodes, &inode->elem);
	inode->sector = sector;
	inode->open_cnt = 1;
	inode->deny_write_cnt = 0;
	inode->removed = false;
	disk_read (filesys_disk, inode->sector, &inode->data);
	return inode;
}

/* INODE를 다시 열어서 돌려줌. */
struct inode *
inode_reopen (struct inode *inode) {
	if (inode != NULL)
		inode->open_cnt++;
	return inode;
}

/* INODE의 inode 번호를 돌려줌. */
disk_sector_t
inode_get_inumber (const struct inode *inode) {
	return inode->sector;
}

/* INODE를 닫고 디스크에 씀.
 * INODE를 가리키는 마지막 참조였으면 메모리를 해제함.
 * INODE가 삭제 표시된 inode이기도 하면 블록도 해제함. */
void
inode_close (struct inode *inode) {
	/* 널 포인터는 무시함. */
	if (inode == NULL)
		return;

	/* 마지막으로 연 쪽이었으면 자원을 해제함. */
	if (--inode->open_cnt == 0) {
		/* inode 리스트에서 빼고 락을 풂. */
		list_remove (&inode->elem);

		/* 삭제된 inode면 블록을 해제함. */
		if (inode->removed) {
			free_map_release (inode->sector, 1);
			free_map_release (inode->data.start,
					bytes_to_sectors (inode->data.length)); 
		}

		free (inode); 
	}
}

/* INODE를 열고 있는 마지막 호출자가 닫을 때 지워지도록 표시함. */
void
inode_remove (struct inode *inode) {
	ASSERT (inode != NULL);
	inode->removed = true;
}

/* INODE의 OFFSET 위치부터 SIZE 바이트를 읽어 BUFFER에 담음.
 * 실제로 읽은 바이트 수를 돌려줌. 오류가 나거나 파일 끝에
 * 닿으면 SIZE보다 적을 수 있음. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) {
	uint8_t *buffer = buffer_;
	off_t bytes_read = 0;
	uint8_t *bounce = NULL;

	while (size > 0) {
		/* 읽을 디스크 섹터, 섹터 안의 시작 바이트 오프셋. */
		disk_sector_t sector_idx = byte_to_sector (inode, offset);
		int sector_ofs = offset % DISK_SECTOR_SIZE;

		/* inode에 남은 바이트, 섹터에 남은 바이트, 둘 중 작은 값. */
		off_t inode_left = inode_length (inode) - offset;
		int sector_left = DISK_SECTOR_SIZE - sector_ofs;
		int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* 이 섹터에서 실제로 복사할 바이트 수. */
		int chunk_size = size < min_left ? size : min_left;
		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) {
			/* 섹터 전체를 호출자의 버퍼로 바로 읽음. */
			disk_read (filesys_disk, sector_idx, buffer + bytes_read); 
		} else {
			/* 섹터를 임시(bounce) 버퍼로 읽은 다음, 일부만
			 * 호출자의 버퍼로 복사함. */
			if (bounce == NULL) {
				bounce = malloc (DISK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}
			disk_read (filesys_disk, sector_idx, bounce);
			memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
		}

		/* 다음으로 진행. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_read += chunk_size;
	}
	free (bounce);

	return bytes_read;
}

/* BUFFER의 SIZE 바이트를 INODE의 OFFSET 위치부터 씀.
 * 실제로 쓴 바이트 수를 돌려줌. 파일 끝에 닿거나 오류가 나면
 * SIZE보다 적을 수 있음.
 * (원래는 파일 끝에 쓰면 inode를 늘려야 하지만, 크기 늘리기는
 * 아직 구현되지 않음.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
		off_t offset) {
	const uint8_t *buffer = buffer_;
	off_t bytes_written = 0;
	uint8_t *bounce = NULL;

	if (inode->deny_write_cnt)
		return 0;

	while (size > 0) {
		/* 쓸 섹터, 섹터 안의 시작 바이트 오프셋. */
		disk_sector_t sector_idx = byte_to_sector (inode, offset);
		int sector_ofs = offset % DISK_SECTOR_SIZE;

		/* inode에 남은 바이트, 섹터에 남은 바이트, 둘 중 작은 값. */
		off_t inode_left = inode_length (inode) - offset;
		int sector_left = DISK_SECTOR_SIZE - sector_ofs;
		int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* 이 섹터에 실제로 쓸 바이트 수. */
		int chunk_size = size < min_left ? size : min_left;
		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) {
			/* 섹터 전체를 디스크에 바로 씀. */
			disk_write (filesys_disk, sector_idx, buffer + bytes_written); 
		} else {
			/* 임시(bounce) 버퍼가 필요함. */
			if (bounce == NULL) {
				bounce = malloc (DISK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}

			/* 쓰려는 부분 앞이나 뒤에 섹터 데이터가 있으면
			   먼저 섹터를 읽어와야 함. 아니면 0으로 채운 섹터에서
			   시작함. */
			if (sector_ofs > 0 || chunk_size < sector_left) 
				disk_read (filesys_disk, sector_idx, bounce);
			else
				memset (bounce, 0, DISK_SECTOR_SIZE);
			memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
			disk_write (filesys_disk, sector_idx, bounce); 
		}

		/* 다음으로 진행. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_written += chunk_size;
	}
	free (bounce);

	return bytes_written;
}

/* INODE에 쓰기를 막음.
   inode를 연 쪽마다 최대 한 번만 부를 수 있음. */
	void
inode_deny_write (struct inode *inode) 
{
	inode->deny_write_cnt++;
	ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* INODE에 다시 쓰기를 허용함.
 * inode_deny_write()를 부른 쪽은 inode를 닫기 전에
 * 반드시 한 번 불러야 함. */
void
inode_allow_write (struct inode *inode) {
	ASSERT (inode->deny_write_cnt > 0);
	ASSERT (inode->deny_write_cnt <= inode->open_cnt);
	inode->deny_write_cnt--;
}

/* INODE 데이터의 길이를 바이트 단위로 돌려줌. */
off_t
inode_length (const struct inode *inode) {
	return inode->data.length;
}
