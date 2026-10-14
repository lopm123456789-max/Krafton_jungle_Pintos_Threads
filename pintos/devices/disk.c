#include "devices/disk.h"
#include <ctype.h>
#include <debug.h>
#include <stdbool.h>
#include <stdio.h>
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/interrupt.h"
#include "threads/synch.h"

/* 이 파일의 코드는 ATA(IDE) 컨트롤러와 통신하는 인터페이스임.
   [ATA-3] 규격을 따르려고 함. */

/* ATA 커맨드 블록 포트 주소. */
#define reg_data(CHANNEL) ((CHANNEL)->reg_base + 0)     /* 데이터. */
#define reg_error(CHANNEL) ((CHANNEL)->reg_base + 1)    /* 에러. */
#define reg_nsect(CHANNEL) ((CHANNEL)->reg_base + 2)    /* 섹터 개수. */
#define reg_lbal(CHANNEL) ((CHANNEL)->reg_base + 3)     /* LBA 0:7. */
#define reg_lbam(CHANNEL) ((CHANNEL)->reg_base + 4)     /* LBA 15:8. */
#define reg_lbah(CHANNEL) ((CHANNEL)->reg_base + 5)     /* LBA 23:16. */
#define reg_device(CHANNEL) ((CHANNEL)->reg_base + 6)   /* 장치/LBA 27:24. */
#define reg_status(CHANNEL) ((CHANNEL)->reg_base + 7)   /* 상태 (읽기 전용). */
#define reg_command(CHANNEL) reg_status (CHANNEL)       /* 명령 (쓰기 전용). */

/* ATA 컨트롤 블록 포트 주소.
   (레거시가 아닌 ATA 컨트롤러까지 지원한다면 이 방식으로는
   부족하겠지만, 지금 용도로는 충분함.) */
#define reg_ctl(CHANNEL) ((CHANNEL)->reg_base + 0x206)  /* 컨트롤 (쓰기 전용). */
#define reg_alt_status(CHANNEL) reg_ctl (CHANNEL)       /* 보조 상태 (읽기 전용). */

/* 보조 상태 레지스터 비트. */
#define STA_BSY 0x80            /* 사용 중(Busy). */
#define STA_DRDY 0x40           /* 장치 준비됨. */
#define STA_DRQ 0x08            /* 데이터 요청. */

/* 컨트롤 레지스터 비트. */
#define CTL_SRST 0x04           /* 소프트웨어 리셋. */

/* 장치 레지스터 비트. */
#define DEV_MBS 0xa0            /* 반드시 설정해야 함. */
#define DEV_LBA 0x40            /* 선형 주소 지정(LBA) 방식. */
#define DEV_DEV 0x10            /* 장치 선택: 0=마스터, 1=슬레이브. */

/* 명령어.
   훨씬 많은 명령이 정의돼 있지만, 여기서 쓰는 일부만
   적어둠. */
#define CMD_IDENTIFY_DEVICE 0xec        /* IDENTIFY DEVICE (장치 식별). */
#define CMD_READ_SECTOR_RETRY 0x20      /* READ SECTOR (재시도 포함). */
#define CMD_WRITE_SECTOR_RETRY 0x30     /* WRITE SECTOR (재시도 포함). */

/* ATA 장치 하나. */
struct disk {
	char name[8];               /* 이름. 예: "hd0:1". */
	struct channel *channel;    /* 이 디스크가 연결된 채널. */
	int dev_no;                 /* 장치 번호. 마스터는 0, 슬레이브는 1. */

	bool is_ata;                /* 1이면 이 장치는 ATA 디스크임. */
	disk_sector_t capacity;     /* 섹터 단위 용량 (is_ata일 때만 유효). */

	long long read_cnt;         /* 읽은 섹터 수. */
	long long write_cnt;        /* 쓴 섹터 수. */
};

/* ATA 채널 (컨트롤러라고도 함).
   채널 하나가 디스크를 최대 두 개까지 제어함. */
struct channel {
	char name[8];               /* 이름. 예: "hd0". */
	uint16_t reg_base;          /* 기본 I/O 포트. */
	uint8_t irq;                /* 사용 중인 인터럽트 번호. */

	struct lock lock;           /* 컨트롤러에 접근하려면 반드시 획득해야 하는 락. */
	bool expecting_interrupt;   /* 인터럽트를 기다리는 중이면 true, 지금 오는
								   인터럽트는 가짜(spurious)라면 false. */
	struct semaphore completion_wait;   /* 인터럽트 핸들러가 up 해주는 세마포어. */

	struct disk devices[2];     /* 이 채널에 연결된 장치들. */
};

/* 일반 PC에 있는 "레거시" ATA 채널 두 개를 지원함. */
#define CHANNEL_CNT 2
static struct channel channels[CHANNEL_CNT];

static void reset_channel (struct channel *);
static bool check_device_type (struct disk *);
static void identify_ata_device (struct disk *);

static void select_sector (struct disk *, disk_sector_t);
static void issue_pio_command (struct channel *, uint8_t command);
static void input_sector (struct channel *, void *);
static void output_sector (struct channel *, const void *);

static void wait_until_idle (const struct disk *);
static bool wait_while_busy (const struct disk *);
static void select_device (const struct disk *);
static void select_device_wait (const struct disk *);

static void interrupt_handler (struct intr_frame *);

/* 디스크 서브시스템을 초기화하고 디스크를 찾음. */
void
disk_init (void) {
	size_t chan_no;

	for (chan_no = 0; chan_no < CHANNEL_CNT; chan_no++) {
		struct channel *c = &channels[chan_no];
		int dev_no;

		/* 채널 초기화. */
		snprintf (c->name, sizeof c->name, "hd%zu", chan_no);
		switch (chan_no) {
			case 0:
				c->reg_base = 0x1f0;
				c->irq = 14 + 0x20;
				break;
			case 1:
				c->reg_base = 0x170;
				c->irq = 15 + 0x20;
				break;
			default:
				NOT_REACHED ();
		}
		lock_init (&c->lock);
		c->expecting_interrupt = false;
		sema_init (&c->completion_wait, 0);

		/* 장치 초기화. */
		for (dev_no = 0; dev_no < 2; dev_no++) {
			struct disk *d = &c->devices[dev_no];
			snprintf (d->name, sizeof d->name, "%s:%d", c->name, dev_no);
			d->channel = c;
			d->dev_no = dev_no;

			d->is_ata = false;
			d->capacity = 0;

			d->read_cnt = d->write_cnt = 0;
		}

		/* 인터럽트 핸들러 등록. */
		intr_register_ext (c->irq, interrupt_handler, c->name);

		/* 하드웨어 리셋. */
		reset_channel (c);

		/* ATA 하드디스크와 다른 장치를 구분함. */
		if (check_device_type (&c->devices[0]))
			check_device_type (&c->devices[1]);

		/* 하드디스크 식별 정보를 읽음. */
		for (dev_no = 0; dev_no < 2; dev_no++)
			if (c->devices[dev_no].is_ata)
				identify_ata_device (&c->devices[dev_no]);
	}

	/* 아래 줄들은 수정하지 말 것. */
	register_disk_inspect_intr ();
}

/* 디스크 통계를 출력함. */
void
disk_print_stats (void) {
	int chan_no;

	for (chan_no = 0; chan_no < CHANNEL_CNT; chan_no++) {
		int dev_no;

		for (dev_no = 0; dev_no < 2; dev_no++) {
			struct disk *d = disk_get (chan_no, dev_no);
			if (d != NULL && d->is_ata)
				printf ("%s: %lld reads, %lld writes\n",
						d->name, d->read_cnt, d->write_cnt);
		}
	}
}

/* CHAN_NO번 채널 안에서 DEV_NO번 디스크를 돌려줌.
   DEV_NO는 0(마스터) 또는 1(슬레이브)임.

   Pintos는 디스크를 이렇게 사용함:
   0:0 - 부트 로더, 커맨드라인 인자, 운영체제 커널
   0:1 - 파일 시스템
   1:0 - 스크래치(임시 작업용)
   1:1 - 스왑
   */
struct disk *
disk_get (int chan_no, int dev_no) {
	ASSERT (dev_no == 0 || dev_no == 1);

	if (chan_no < (int) CHANNEL_CNT) {
		struct disk *d = &channels[chan_no].devices[dev_no];
		if (d->is_ata)
			return d;
	}
	return NULL;
}

/* 디스크 D의 크기를 DISK_SECTOR_SIZE 바이트짜리 섹터 개수로
   돌려줌. */
disk_sector_t
disk_size (struct disk *d) {
	ASSERT (d != NULL);

	return d->capacity;
}

/* 디스크 D의 SEC_NO번 섹터를 BUFFER로 읽어옴.
   BUFFER는 DISK_SECTOR_SIZE 바이트를 담을 공간이 있어야 함.
   디스크 접근은 내부에서 동기화하므로, 밖에서 디스크별로
   따로 락을 걸 필요 없음. */
void
disk_read (struct disk *d, disk_sector_t sec_no, void *buffer) {
	struct channel *c;

	ASSERT (d != NULL);
	ASSERT (buffer != NULL);

	c = d->channel;
	lock_acquire (&c->lock);
	select_sector (d, sec_no);
	issue_pio_command (c, CMD_READ_SECTOR_RETRY);
	sema_down (&c->completion_wait);
	if (!wait_while_busy (d))
		PANIC ("%s: disk read failed, sector=%"PRDSNu, d->name, sec_no);
	input_sector (c, buffer);
	d->read_cnt++;
	lock_release (&c->lock);
}

/* BUFFER의 내용을 디스크 D의 SEC_NO번 섹터에 씀.
   BUFFER는 DISK_SECTOR_SIZE 바이트를 담고 있어야 함.
   디스크가 데이터를 받았다고 응답한 뒤에 반환함.
   디스크 접근은 내부에서 동기화하므로, 밖에서 디스크별로
   따로 락을 걸 필요 없음. */
void
disk_write (struct disk *d, disk_sector_t sec_no, const void *buffer) {
	struct channel *c;

	ASSERT (d != NULL);
	ASSERT (buffer != NULL);

	c = d->channel;
	lock_acquire (&c->lock);
	select_sector (d, sec_no);
	issue_pio_command (c, CMD_WRITE_SECTOR_RETRY);
	if (!wait_while_busy (d))
		PANIC ("%s: disk write failed, sector=%"PRDSNu, d->name, sec_no);
	output_sector (c, buffer);
	sema_down (&c->completion_wait);
	d->write_cnt++;
	lock_release (&c->lock);
}

/* 디스크 감지 및 식별. */

static void print_ata_string (char *string, size_t size);

/* ATA 채널을 리셋하고, 연결된 장치들이 리셋을 마칠 때까지
   기다림. */
static void
reset_channel (struct channel *c) {
	bool present[2];
	int dev_no;

	/* ATA 리셋 순서는 어떤 장치가 있는지에 따라 달라서,
	   먼저 장치가 있는지부터 확인함. */
	for (dev_no = 0; dev_no < 2; dev_no++) {
		struct disk *d = &c->devices[dev_no];

		select_device (d);

		outb (reg_nsect (c), 0x55);
		outb (reg_lbal (c), 0xaa);

		outb (reg_nsect (c), 0xaa);
		outb (reg_lbal (c), 0x55);

		outb (reg_nsect (c), 0x55);
		outb (reg_lbal (c), 0xaa);

		present[dev_no] = (inb (reg_nsect (c)) == 0x55
				&& inb (reg_lbal (c)) == 0xaa);
	}

	/* 소프트 리셋을 실행함. 부수 효과로 장치 0이 선택됨.
	   인터럽트도 함께 켬. */
	outb (reg_ctl (c), 0);
	timer_usleep (10);
	outb (reg_ctl (c), CTL_SRST);
	timer_usleep (10);
	outb (reg_ctl (c), 0);

	timer_msleep (150);

	/* 장치 0의 BSY가 풀릴 때까지 기다림. */
	if (present[0]) {
		select_device (&c->devices[0]);
		wait_while_busy (&c->devices[0]);
	}

	/* 장치 1의 BSY가 풀릴 때까지 기다림. */
	if (present[1]) {
		int i;

		select_device (&c->devices[1]);
		for (i = 0; i < 3000; i++) {
			if (inb (reg_nsect (c)) == 1 && inb (reg_lbal (c)) == 1)
				break;
			timer_msleep (10);
		}
		wait_while_busy (&c->devices[1]);
	}
}

/* 장치 D가 ATA 디스크인지 확인하고 D의 is_ata 값을 설정함.
   D가 장치 0(마스터)이면, 이 채널에 슬레이브(장치 1)가
   있을 수 있을 때 true를 돌려줌.
   D가 장치 1(슬레이브)이면 반환값은 의미 없음. */
static bool
check_device_type (struct disk *d) {
	struct channel *c = d->channel;
	uint8_t error, lbam, lbah, status;

	select_device (d);

	error = inb (reg_error (c));
	lbam = inb (reg_lbam (c));
	lbah = inb (reg_lbah (c));
	status = inb (reg_status (c));

	if ((error != 1 && (error != 0x81 || d->dev_no == 1))
			|| (status & STA_DRDY) == 0
			|| (status & STA_BSY) != 0) {
		d->is_ata = false;
		return error != 0x81;
	} else {
		d->is_ata = (lbam == 0 && lbah == 0) || (lbam == 0x3c && lbah == 0xc3);
		return true;
	}
}

/* 디스크 D에 IDENTIFY DEVICE 명령을 보내고 응답을 읽음.
   결과를 바탕으로 D의 용량(capacity)을 설정하고,
   디스크 정보를 콘솔에 출력함. */
static void
identify_ata_device (struct disk *d) {
	struct channel *c = d->channel;
	uint16_t id[DISK_SECTOR_SIZE / 2];

	ASSERT (d->is_ata);

	/* IDENTIFY DEVICE 명령을 보내고, 장치 응답이 준비됐다는
	   인터럽트를 기다린 뒤, 데이터를 버퍼로 읽어옴. */
	select_device_wait (d);
	issue_pio_command (c, CMD_IDENTIFY_DEVICE);
	sema_down (&c->completion_wait);
	if (!wait_while_busy (d)) {
		d->is_ata = false;
		return;
	}
	input_sector (c, id);

	/* 용량 계산. */
	d->capacity = id[60] | ((uint32_t) id[61] << 16);

	/* 식별 메시지 출력. */
	printf ("%s: detected %'"PRDSNu" sector (", d->name, d->capacity);
	if (d->capacity > 1024 / DISK_SECTOR_SIZE * 1024 * 1024)
		printf ("%"PRDSNu" GB",
				d->capacity / (1024 / DISK_SECTOR_SIZE * 1024 * 1024));
	else if (d->capacity > 1024 / DISK_SECTOR_SIZE * 1024)
		printf ("%"PRDSNu" MB", d->capacity / (1024 / DISK_SECTOR_SIZE * 1024));
	else if (d->capacity > 1024 / DISK_SECTOR_SIZE)
		printf ("%"PRDSNu" kB", d->capacity / (1024 / DISK_SECTOR_SIZE));
	else
		printf ("%"PRDSNu" byte", d->capacity * DISK_SECTOR_SIZE);
	printf (") disk, model \"");
	print_ata_string ((char *) &id[27], 40);
	printf ("\", serial \"");
	print_ata_string ((char *) &id[10], 20);
	printf ("\"\n");
}

/* SIZE 바이트짜리 STRING을 특이한 형식으로 출력함:
   바이트 두 개씩 순서가 뒤집혀 있음. 끝에 붙은
   공백이나 널 문자는 출력하지 않음. */
static void
print_ata_string (char *string, size_t size) {
	size_t i;

	/* 공백도 널도 아닌 마지막 문자를 찾음. */
	for (; size > 0; size--) {
		int c = string[(size - 1) ^ 1];
		if (c != '\0' && !isspace (c))
			break;
	}

	/* 출력. */
	for (i = 0; i < size; i++)
		printf ("%c", string[i ^ 1]);
}

/* 장치 D를 선택하고 준비될 때까지 기다린 뒤, 디스크의 섹터
   선택 레지스터에 SEC_NO를 씀. (LBA 모드를 사용함.) */
static void
select_sector (struct disk *d, disk_sector_t sec_no) {
	struct channel *c = d->channel;

	ASSERT (sec_no < d->capacity);
	ASSERT (sec_no < (1UL << 28));

	select_device_wait (d);
	outb (reg_nsect (c), 1);
	outb (reg_lbal (c), sec_no);
	outb (reg_lbam (c), sec_no >> 8);
	outb (reg_lbah (c), (sec_no >> 16));
	outb (reg_device (c),
			DEV_MBS | DEV_LBA | (d->dev_no == 1 ? DEV_DEV : 0) | (sec_no >> 24));
}

/* 채널 C에 COMMAND를 쓰고, 완료 인터럽트를 받을 준비를 함. */
static void
issue_pio_command (struct channel *c, uint8_t command) {
	/* 인터럽트가 켜져 있어야 함. 아니면 완료 핸들러가
	   세마포어를 up 해주지 못함. */
	ASSERT (intr_get_level () == INTR_ON);

	c->expecting_interrupt = true;
	outb (reg_command (c), command);
}

/* PIO 모드로 채널 C의 데이터 레지스터에서 섹터 하나를 읽어
   SECTOR에 담음. SECTOR는 DISK_SECTOR_SIZE 바이트를 담을
   공간이 있어야 함. */
static void
input_sector (struct channel *c, void *sector) {
	insw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
}

/* PIO 모드로 SECTOR를 채널 C의 데이터 레지스터에 씀.
   SECTOR는 DISK_SECTOR_SIZE 바이트를 담고 있어야 함. */
static void
output_sector (struct channel *c, const void *sector) {
	outsw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
}

/* 저수준 ATA 기본 함수들. */

/* 컨트롤러가 한가해질 때까지 최대 10초 기다림.
   즉, 상태 레지스터의 BSY와 DRQ 비트가 풀릴 때까지 기다림.

   부수 효과로, 상태 레지스터를 읽으면 대기 중인
   인터럽트가 지워짐. */
static void
wait_until_idle (const struct disk *d) {
	int i;

	for (i = 0; i < 1000; i++) {
		if ((inb (reg_status (d->channel)) & (STA_BSY | STA_DRQ)) == 0)
			return;
		timer_usleep (10);
	}

	printf ("%s: idle timeout\n", d->name);
}

/* 디스크 D의 BSY가 풀릴 때까지 최대 30초 기다린 뒤,
   DRQ 비트 상태를 돌려줌.
   ATA 규격상 디스크 리셋이 그만큼 걸릴 수 있다고 함. */
static bool
wait_while_busy (const struct disk *d) {
	struct channel *c = d->channel;
	int i;

	for (i = 0; i < 3000; i++) {
		if (i == 700)
			printf ("%s: busy, waiting...", d->name);
		if (!(inb (reg_alt_status (c)) & STA_BSY)) {
			if (i >= 700)
				printf ("ok\n");
			return (inb (reg_alt_status (c)) & STA_DRQ) != 0;
		}
		timer_msleep (10);
	}

	printf ("failed\n");
	return false;
}

/* D의 채널을 설정해서 D를 선택된 디스크로 만듦. */
static void
select_device (const struct disk *d) {
	struct channel *c = d->channel;
	uint8_t dev = DEV_MBS;
	if (d->dev_no == 1)
		dev |= DEV_DEV;
	outb (reg_device (c), dev);
	inb (reg_alt_status (c));
	timer_nsleep (400);
}

/* select_device()처럼 채널에서 디스크 D를 선택하되,
   선택 전후로 채널이 한가해질 때까지 기다림. */
static void
select_device_wait (const struct disk *d) {
	wait_until_idle (d);
	select_device (d);
	wait_until_idle (d);
}

/* ATA 인터럽트 핸들러. */
static void
interrupt_handler (struct intr_frame *f) {
	struct channel *c;

	for (c = channels; c < channels + CHANNEL_CNT; c++)
		if (f->vec_no == c->irq) {
			if (c->expecting_interrupt) {
				inb (reg_status (c));               /* 인터럽트 수신 확인. */
				sema_up (&c->completion_wait);      /* 기다리는 스레드를 깨움. */
			} else
				printf ("%s: unexpected interrupt\n", c->name);
			return;
		}

	NOT_REACHED ();
}

static void
inspect_read_cnt (struct intr_frame *f) {
	struct disk * d = disk_get (f->R.rdx, f->R.rcx);
	f->R.rax = d->read_cnt;
}

static void
inspect_write_cnt (struct intr_frame *f) {
	struct disk * d = disk_get (f->R.rdx, f->R.rcx);
	f->R.rax = d->write_cnt;
}

/* 디스크 읽기/쓰기 횟수 테스트용 도구. int 0x43, int 0x44로 호출함.
 * 입력:
 *   @RDX - 확인할 디스크의 chan_no
 *   @RCX - 확인할 디스크의 dev_no
 * 출력:
 *   @RAX - 디스크의 읽기/쓰기 횟수. */
void
register_disk_inspect_intr (void) {
	intr_register_int (0x43, 3, INTR_OFF, inspect_read_cnt, "Inspect Disk Read Count");
	intr_register_int (0x44, 3, INTR_OFF, inspect_write_cnt, "Inspect Disk Write Count");
}
