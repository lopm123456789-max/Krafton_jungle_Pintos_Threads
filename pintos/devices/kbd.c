#include "devices/kbd.h"
#include <ctype.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "devices/input.h"
#include "threads/interrupt.h"
#include "threads/io.h"

/* 키보드 데이터 레지스터 포트. */
#define DATA_REG 0x60

/* 현재 Shift 계열 키 상태.
   눌려 있으면 true, 아니면 false. */
static bool left_shift, right_shift;    /* 왼쪽/오른쪽 Shift 키. */
static bool left_alt, right_alt;        /* 왼쪽/오른쪽 Alt 키. */
static bool left_ctrl, right_ctrl;      /* 왼쪽/오른쪽 Ctrl 키. */

/* Caps Lock 상태.
   켜져 있으면 true, 꺼져 있으면 false. */
static bool caps_lock;

/* 눌린 키 개수. */
static int64_t key_cnt;

static intr_handler_func keyboard_interrupt;

/* 키보드를 초기화함. */
void
kbd_init (void) {
	intr_register_ext (0x21, keyboard_interrupt, "8042 Keyboard");
}

/* 키보드 통계를 출력함. */
void
kbd_print_stats (void) {
	printf ("Keyboard: %lld keys pressed\n", key_cnt);
}

/* 연속된 스캔코드 묶음을 문자로 바꾸는 표. */
struct keymap {
	uint8_t first_scancode;     /* 첫 번째 스캔코드. */
	const char *chars;          /* chars[0]은 first_scancode,
								   chars[1]은 first_scancode + 1에 해당하고,
								   문자열 끝까지 이런 식으로 이어짐. */
};

/* Shift 키를 눌렀든 안 눌렀든 같은 문자가 나오는 키들.
   알파벳 대소문자는 예외이며 다른 곳에서 처리함. */
static const struct keymap invariant_keymap[] = {
	{0x01, "\033"},
	{0x0e, "\b"},
	{0x0f, "\tQWERTYUIOP"},
	{0x1c, "\r"},
	{0x1e, "ASDFGHJKL"},
	{0x2c, "ZXCVBNM"},
	{0x37, "*"},
	{0x39, " "},
	{0, NULL},
};

/* Shift 없이 눌렀을 때의 문자. Shift 여부에 따라
   문자가 달라지는 키만 해당함. */
static const struct keymap unshifted_keymap[] = {
	{0x02, "1234567890-="},
	{0x1a, "[]"},
	{0x27, ";'`"},
	{0x2b, "\\"},
	{0x33, ",./"},
	{0, NULL},
};

/* Shift와 함께 눌렀을 때의 문자. Shift 여부에 따라
   문자가 달라지는 키만 해당함. */
static const struct keymap shifted_keymap[] = {
	{0x02, "!@#$%^&*()_+"},
	{0x1a, "{}"},
	{0x27, ":\"~"},
	{0x2b, "|"},
	{0x33, "<>?"},
	{0, NULL},
};

static bool map_key (const struct keymap[], unsigned scancode, uint8_t *);

static void
keyboard_interrupt (struct intr_frame *args UNUSED) {
	/* Shift 계열 키 상태. */
	bool shift = left_shift || right_shift;
	bool alt = left_alt || right_alt;
	bool ctrl = left_ctrl || right_ctrl;

	/* 키보드 스캔코드. */
	unsigned code;

	/* 키를 눌렀으면 false, 뗐으면 true. */
	bool release;

	/* `code'에 해당하는 문자. */
	uint8_t c;

	/* 스캔코드를 읽음. 접두 코드면 두 번째 바이트까지 읽음. */
	code = inb (DATA_REG);
	if (code == 0xe0)
		code = (code << 8) | inb (DATA_REG);

	/* 0x80 비트로 키 누름과 키 뗌을 구분함
	   (접두 코드가 있어도 마찬가지임). */
	release = (code & 0x80) != 0;
	code &= ~0x80u;

	/* 키 해석. */
	if (code == 0x3a) {
		/* Caps Lock. */
		if (!release)
			caps_lock = !caps_lock;
	} else if (map_key (invariant_keymap, code, &c)
			|| (!shift && map_key (unshifted_keymap, code, &c))
			|| (shift && map_key (shifted_keymap, code, &c))) {
		/* 일반 문자. */
		if (!release) {
			/* Ctrl, Shift 처리.
			   Ctrl이 Shift보다 우선함. */
			if (ctrl && c >= 0x40 && c < 0x60) {
				/* A는 0x41, Ctrl+A는 0x01, 이런 식임. */
				c -= 0x40;
			} else if (shift == caps_lock)
				c = tolower (c);

			/* Alt는 최상위 비트를 켜서 처리함.
			   여기의 0x80은 키 누름/뗌을 구분하는 0x80과는
			   상관없음. */
			if (alt)
				c += 0x80;

			/* 키보드 버퍼에 추가함. */
			if (!input_full ()) {
				key_cnt++;
				input_putc (c);
			}
		}
	} else {
		/* 키 코드를 Shift 상태 변수에 대응시키는 표. */
		struct shift_key {
			unsigned scancode;
			bool *state_var;
		};

		/* Shift 계열 키 표. */
		static const struct shift_key shift_keys[] = {
			{  0x2a, &left_shift},
			{  0x36, &right_shift},
			{  0x38, &left_alt},
			{0xe038, &right_alt},
			{  0x1d, &left_ctrl},
			{0xe01d, &right_ctrl},
			{0,      NULL},
		};

		const struct shift_key *key;

		/* 표를 훑어봄. */
		for (key = shift_keys; key->scancode != 0; key++)
			if (key->scancode == code) {
				*key->state_var = !release;
				break;
			}
	}
}

/* 키맵 배열 K에서 SCANCODE를 찾음.
   찾으면 *C에 해당 문자를 넣고 true를 돌려줌.
   못 찾으면 false를 돌려주고 C는 건드리지 않음. */
static bool
map_key (const struct keymap k[], unsigned scancode, uint8_t *c) {
	for (; k->first_scancode != 0; k++)
		if (scancode >= k->first_scancode
				&& scancode < k->first_scancode + strlen (k->chars)) {
			*c = k->chars[scancode - k->first_scancode];
			return true;
		}

	return false;
}
