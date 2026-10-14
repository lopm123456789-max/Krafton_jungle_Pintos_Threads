#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

/* 이중 연결 리스트.
 *
 * 이 이중 연결 리스트 구현은 동적 할당 메모리를 쓰지 않음.
 * 대신 리스트 원소가 될 수 있는 구조체마다 struct list_elem
 * 멤버를 품고 있어야 함. 모든 리스트 함수는 이 `struct
 * list_elem'을 다룸. list_entry 매크로로 struct list_elem에서
 * 그걸 품고 있는 구조체 객체로 되돌아갈 수 있음.

 * 예를 들어 `struct foo'의 리스트가 필요하다면, `struct foo'
 * 안에 이렇게 `struct list_elem' 멤버를 넣어야 함:

 * struct foo {
 *   struct list_elem elem;
 *   int bar;
 *   ...other members...
 * };

 * 그러면 `struct foo'의 리스트를 이렇게 선언하고 초기화할 수 있음:

 * struct list foo_list;

 * list_init (&foo_list);

 * 순회는 struct list_elem에서 그걸 감싸는 구조체로 되돌아가야
 * 하는 대표적인 경우임. foo_list로 예를 들면:

 * struct list_elem *e;

 * for (e = list_begin (&foo_list); e != list_end (&foo_list);
 * e = list_next (e)) {
 *   struct foo *f = list_entry (e, struct foo, elem);
 *   ...do something with f...
 * }

 * 실제 사용 예시는 소스 곳곳에서 볼 수 있음. 예를 들어 threads
 * 폴더의 malloc.c, palloc.c, thread.c가 모두 리스트를 씀.

 * 이 리스트의 인터페이스는 C++ STL의 list<> 템플릿을 본떴음.
 * list<>에 익숙하다면 쉽게 쓸 수 있을 것임. 다만 이 리스트는
 * 타입 검사를 *전혀* 하지 않고, 그 밖의 정확성 검사도 거의 못 함.
 * 실수하면 그대로 버그로 돌아옴.

 * 리스트 용어 정리:

 * - "front": 리스트의 첫 번째 원소. 빈 리스트에서는 정의되지
 * 않음. list_front()가 돌려줌.

 * - "back": 리스트의 마지막 원소. 빈 리스트에서는 정의되지
 * 않음. list_back()이 돌려줌.

 * - "tail": 마지막 원소 바로 뒤에 있다고 보는 가상의 원소.
 * 빈 리스트에서도 정의됨. list_end()가 돌려줌. 앞에서 뒤로
 * 순회할 때 끝 표시(sentinel)로 씀.

 * - "beginning": 비어 있지 않은 리스트에서는 front, 빈
 * 리스트에서는 tail. list_begin()이 돌려줌. 앞에서 뒤로
 * 순회할 때 시작점으로 씀.

 * - "head": 첫 번째 원소 바로 앞에 있다고 보는 가상의 원소.
 * 빈 리스트에서도 정의됨. list_rend()가 돌려줌. 뒤에서 앞으로
 * 순회할 때 끝 표시로 씀.

 * - "reverse beginning": 비어 있지 않은 리스트에서는 back, 빈
 * 리스트에서는 head. list_rbegin()이 돌려줌. 뒤에서 앞으로
 * 순회할 때 시작점으로 씀.
 *
 * - "interior element": head나 tail이 아닌 원소, 즉 진짜 리스트
 * 원소. 빈 리스트에는 interior element가 없음.*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 리스트 원소. */
struct list_elem {
	struct list_elem *prev;     /* 이전 리스트 원소. */
	struct list_elem *next;     /* 다음 리스트 원소. */
};

/* 리스트. */
struct list {
	struct list_elem head;      /* 리스트 head. */
	struct list_elem tail;      /* 리스트 tail. */
};

/* 리스트 원소 LIST_ELEM의 포인터를, LIST_ELEM을 품고 있는
   구조체의 포인터로 바꿈. 바깥 구조체 이름 STRUCT와 리스트
   원소의 멤버 이름 MEMBER를 넘겨줘야 함. 예시는 파일 맨 위의
   큰 주석 참고. */
#define list_entry(LIST_ELEM, STRUCT, MEMBER)           \
	((STRUCT *) ((uint8_t *) &(LIST_ELEM)->next     \
		- offsetof (STRUCT, MEMBER.next)))

void list_init (struct list *);

/* 리스트 순회. */
struct list_elem *list_begin (struct list *);
struct list_elem *list_next (struct list_elem *);
struct list_elem *list_end (struct list *);

struct list_elem *list_rbegin (struct list *);
struct list_elem *list_prev (struct list_elem *);
struct list_elem *list_rend (struct list *);

struct list_elem *list_head (struct list *);
struct list_elem *list_tail (struct list *);

/* 리스트 삽입. */
void list_insert (struct list_elem *, struct list_elem *);
void list_splice (struct list_elem *before,
		struct list_elem *first, struct list_elem *last);
void list_push_front (struct list *, struct list_elem *);
void list_push_back (struct list *, struct list_elem *);

/* 리스트 제거. */
struct list_elem *list_remove (struct list_elem *);
struct list_elem *list_pop_front (struct list *);
struct list_elem *list_pop_back (struct list *);

/* 리스트 원소. */
struct list_elem *list_front (struct list *);
struct list_elem *list_back (struct list *);

/* 리스트 속성. */
size_t list_size (struct list *);
bool list_empty (struct list *);

/* 기타. */
void list_reverse (struct list *);

/* 보조 데이터 AUX를 받아서 두 리스트 원소 A와 B의 값을 비교함.
   A가 B보다 작으면 true, A가 B보다 크거나 같으면 false를
   돌려줌. */
typedef bool list_less_func (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux);

/* 원소가 정렬된 리스트에 대한 연산. */
void list_sort (struct list *,
                list_less_func *, void *aux);
void list_insert_ordered (struct list *, struct list_elem *,
                          list_less_func *, void *aux);
void list_unique (struct list *, struct list *duplicates,
                  list_less_func *, void *aux);

/* 최댓값과 최솟값. */
struct list_elem *list_max (struct list *, list_less_func *, void *aux);
struct list_elem *list_min (struct list *, list_less_func *, void *aux);

#endif /* lib/kernel/list.h */
