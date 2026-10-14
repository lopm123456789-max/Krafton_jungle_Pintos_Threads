#ifndef __LIB_KERNEL_HASH_H
#define __LIB_KERNEL_HASH_H

/* 해시 테이블.
 *
 * 이 자료구조는 Project 3의 Tour of Pintos에 자세히 설명돼 있음.
 *
 * 체이닝 방식의 표준 해시 테이블임. 테이블에서 원소를 찾을 때는
 * 원소 데이터로 해시 함수를 계산하고, 그 값을 이중 연결 리스트
 * 배열의 인덱스로 써서 해당 리스트를 처음부터 훑음.
 *
 * 체인 리스트는 동적 할당을 쓰지 않음. 대신 해시에 들어갈 수 있는
 * 구조체마다 struct hash_elem 멤버를 품고 있어야 함. 모든 해시
 * 함수는 이 `struct hash_elem'을 다룸. hash_entry 매크로로
 * struct hash_elem에서 그걸 품고 있는 구조체 객체로 되돌아갈 수
 * 있음. 연결 리스트 구현과 같은 기법임. 자세한 설명은
 * lib/kernel/list.h 참고. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h"

/* 해시 원소. */
struct hash_elem {
	struct list_elem list_elem;
};

/* 해시 원소 HASH_ELEM의 포인터를, HASH_ELEM을 품고 있는
 * 구조체의 포인터로 바꿈. 바깥 구조체 이름 STRUCT와 해시 원소의
 * 멤버 이름 MEMBER를 넘겨줘야 함. 예시는 파일 맨 위의 큰 주석
 * 참고. */
#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
	((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
		- offsetof (STRUCT, MEMBER.list_elem)))

/* 보조 데이터 AUX를 받아서 해시 원소 E의 해시 값을 계산해
 * 돌려줌. */
typedef uint64_t hash_hash_func (const struct hash_elem *e, void *aux);

/* 보조 데이터 AUX를 받아서 두 해시 원소 A와 B의 값을 비교함.
 * A가 B보다 작으면 true, A가 B보다 크거나 같으면 false. */
typedef bool hash_less_func (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

/* 보조 데이터 AUX를 받아서 해시 원소 E에 어떤 작업을 함. */
typedef void hash_action_func (struct hash_elem *e, void *aux);

/* 해시 테이블. */
struct hash {
	size_t elem_cnt;            /* 테이블 안의 원소 개수. */
	size_t bucket_cnt;          /* 버킷 개수. 2의 거듭제곱임. */
	struct list *buckets;       /* `bucket_cnt'개 리스트의 배열. */
	hash_hash_func *hash;       /* 해시 함수. */
	hash_less_func *less;       /* 비교 함수. */
	void *aux;                  /* `hash'와 `less'에 넘길 보조 데이터. */
};

/* 해시 테이블 반복자. */
struct hash_iterator {
	struct hash *hash;          /* 해시 테이블. */
	struct list *bucket;        /* 현재 버킷. */
	struct hash_elem *elem;     /* 현재 버킷 안의 현재 해시 원소. */
};

/* 기본 생명 주기. */
bool hash_init (struct hash *, hash_hash_func *, hash_less_func *, void *aux);
void hash_clear (struct hash *, hash_action_func *);
void hash_destroy (struct hash *, hash_action_func *);

/* 검색, 삽입, 삭제. */
struct hash_elem *hash_insert (struct hash *, struct hash_elem *);
struct hash_elem *hash_replace (struct hash *, struct hash_elem *);
struct hash_elem *hash_find (struct hash *, struct hash_elem *);
struct hash_elem *hash_delete (struct hash *, struct hash_elem *);

/* 순회. */
void hash_apply (struct hash *, hash_action_func *);
void hash_first (struct hash_iterator *, struct hash *);
struct hash_elem *hash_next (struct hash_iterator *);
struct hash_elem *hash_cur (struct hash_iterator *);

/* 정보. */
size_t hash_size (struct hash *);
bool hash_empty (struct hash *);

/* 예시 해시 함수. */
uint64_t hash_bytes (const void *, size_t);
uint64_t hash_string (const char *);
uint64_t hash_int (int);

#endif /* lib/kernel/hash.h */
