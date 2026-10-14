#include "list.h"
#include "../debug.h"

/* 이 이중 연결 리스트에는 헤더 원소가 두 개 있음: 첫 원소
   바로 앞의 "head"와 마지막 원소 바로 뒤의 "tail". 앞쪽 헤더의
   `prev' 링크와 뒤쪽 헤더의 `next' 링크는 널임. 나머지 두 링크는
   리스트의 내부 원소들을 거쳐 서로를 향함.

   빈 리스트는 이렇게 생김:

   +------+     +------+
   <---| head |<--->| tail |--->
   +------+     +------+

   원소가 두 개 들어 있는 리스트는 이렇게 생김:

   +------+     +-------+     +-------+     +------+
   <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
   +------+     +-------+     +-------+     +------+

   이렇게 대칭 구조로 두면 리스트 처리에서 특수한 경우가 많이
   사라짐. 예를 들어 list_remove()를 보면 포인터 대입 두 번만 하고
   조건문이 하나도 없음. 헤더 원소가 없을 때보다 훨씬 간단함.

   (각 헤더 원소는 포인터를 하나씩만 쓰므로, 사실 이 단순함을
   잃지 않고 헤더 하나로 합칠 수도 있음. 하지만 헤더를 두 개로
   나눠두면 일부 연산에서 약간의 검사를 할 수 있어서 유용함.) */

static bool is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) UNUSED;

/* ELEM이 head면 true, 아니면 false. */
static inline bool
is_head (struct list_elem *elem) {
	return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* ELEM이 내부 원소면 true, 아니면 false. */
static inline bool
is_interior (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* ELEM이 tail이면 true, 아니면 false. */
static inline bool
is_tail (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* LIST를 빈 리스트로 초기화함. */
void
list_init (struct list *list) {
	ASSERT (list != NULL);
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

/* LIST의 시작(beginning)을 돌려줌.  */
struct list_elem *
list_begin (struct list *list) {
	ASSERT (list != NULL);
	return list->head.next;
}

/* 리스트에서 ELEM 다음 원소를 돌려줌. ELEM이 리스트의
   마지막 원소면 리스트의 tail을 돌려줌. ELEM 자체가 tail이면
   결과를 보장할 수 없음. */
struct list_elem *
list_next (struct list_elem *elem) {
	ASSERT (is_head (elem) || is_interior (elem));
	return elem->next;
}

/* LIST의 tail을 돌려줌.

   list_end()는 리스트를 앞에서 뒤로 순회할 때 자주 씀. 예시는
   list.h 맨 위의 큰 주석 참고. */
struct list_elem *
list_end (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* LIST를 뒤에서 앞으로 역순 순회할 때 쓰는 역방향 시작
   (reverse beginning)을 돌려줌. */
struct list_elem *
list_rbegin (struct list *list) {
	ASSERT (list != NULL);
	return list->tail.prev;
}

/* 리스트에서 ELEM 이전 원소를 돌려줌. ELEM이 리스트의 첫
   원소면 리스트의 head를 돌려줌. ELEM 자체가 head면 결과를
   보장할 수 없음. */
struct list_elem *
list_prev (struct list_elem *elem) {
	ASSERT (is_interior (elem) || is_tail (elem));
	return elem->prev;
}

/* LIST의 head를 돌려줌.

   list_rend()는 리스트를 뒤에서 앞으로 역순 순회할 때 자주 씀.
   list.h 맨 위의 예시를 이어서 보면 보통 이렇게 씀:

   for (e = list_rbegin (&foo_list); e != list_rend (&foo_list);
   e = list_prev (e))
   {
   struct foo *f = list_entry (e, struct foo, elem);
   ...do something with f...
   }
   */
struct list_elem *
list_rend (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* LIST의 head를 돌려줌.

   list_head()는 리스트를 순회하는 다른 방식에 쓸 수 있음. 예:

   e = list_head (&list);
   while ((e = list_next (e)) != list_end (&list))
   {
   ...
   }
   */
struct list_elem *
list_head (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* LIST의 tail을 돌려줌. */
struct list_elem *
list_tail (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* ELEM을 BEFORE 바로 앞에 넣음. BEFORE는 내부 원소이거나
   tail일 수 있음. tail이면 list_push_back()과 같음. */
void
list_insert (struct list_elem *before, struct list_elem *elem) {
	ASSERT (is_interior (before) || is_tail (before));
	ASSERT (elem != NULL);

	elem->prev = before->prev;
	elem->next = before;
	before->prev->next = elem;
	before->prev = elem;
}

/* FIRST부터 LAST 직전까지의 원소들을 현재 리스트에서 빼서
   BEFORE 바로 앞에 넣음. BEFORE는 내부 원소이거나 tail일 수
   있음. */
void
list_splice (struct list_elem *before,
		struct list_elem *first, struct list_elem *last) {
	ASSERT (is_interior (before) || is_tail (before));
	if (first == last)
		return;
	last = list_prev (last);

	ASSERT (is_interior (first));
	ASSERT (is_interior (last));

	/* FIRST...LAST를 현재 리스트에서 깔끔하게 떼어냄. */
	first->prev->next = last->next;
	last->next->prev = first->prev;

	/* FIRST...LAST를 새 리스트에 이어 붙임. */
	first->prev = before->prev;
	last->next = before;
	before->prev->next = first;
	before->prev = last;
}

/* ELEM을 LIST의 맨 앞에 넣어서 LIST의 front가 되게 함. */
void
list_push_front (struct list *list, struct list_elem *elem) {
	list_insert (list_begin (list), elem);
}

/* ELEM을 LIST의 맨 뒤에 넣어서 LIST의 back이 되게 함. */
void
list_push_back (struct list *list, struct list_elem *elem) {
	list_insert (list_end (list), elem);
}

/* ELEM을 리스트에서 빼고, ELEM 바로 다음에 있던 원소를
   돌려줌. ELEM이 리스트에 없으면 결과를 보장할 수 없음.

   뺀 뒤의 ELEM을 리스트 원소처럼 다루면 안전하지 않음.
   특히 뺀 ELEM에 list_next()나 list_prev()를 쓰면 결과를
   보장할 수 없음. 그래서 리스트 원소를 지우는 단순한 루프는
   실패함:

   ** 이렇게 하지 말 것 **
   for (e = list_begin (&list); e != list_end (&list); e = list_next (e))
   {
   ...do something with e...
   list_remove (e);
   }
   ** 이렇게 하지 말 것 **

   리스트를 순회하면서 원소를 지우는 올바른 방법 하나:

   for (e = list_begin (&list); e != list_end (&list); e = list_remove (e))
   {
   ...do something with e...
   }

   리스트 원소를 free() 해야 한다면 더 조심해야 함. 그런 경우에도
   동작하는 다른 방법:

   while (!list_empty (&list))
   {
   struct list_elem *e = list_pop_front (&list);
   ...do something with e...
   }
   */
struct list_elem *
list_remove (struct list_elem *elem) {
	ASSERT (is_interior (elem));
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	return elem->next;
}

/* LIST의 front 원소를 빼서 돌려줌.
   빼기 전에 LIST가 비어 있으면 결과를 보장할 수 없음. */
struct list_elem *
list_pop_front (struct list *list) {
	struct list_elem *front = list_front (list);
	list_remove (front);
	return front;
}

/* LIST의 back 원소를 빼서 돌려줌.
   빼기 전에 LIST가 비어 있으면 결과를 보장할 수 없음. */
struct list_elem *
list_pop_back (struct list *list) {
	struct list_elem *back = list_back (list);
	list_remove (back);
	return back;
}

/* LIST의 front 원소를 돌려줌.
   LIST가 비어 있으면 결과를 보장할 수 없음. */
struct list_elem *
list_front (struct list *list) {
	ASSERT (!list_empty (list));
	return list->head.next;
}

/* LIST의 back 원소를 돌려줌.
   LIST가 비어 있으면 결과를 보장할 수 없음. */
struct list_elem *
list_back (struct list *list) {
	ASSERT (!list_empty (list));
	return list->tail.prev;
}

/* LIST의 원소 수를 돌려줌.
   원소 수에 대해 O(n) 시간이 걸림. */
size_t
list_size (struct list *list) {
	struct list_elem *e;
	size_t cnt = 0;

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		cnt++;
	return cnt;
}

/* LIST가 비어 있으면 true, 아니면 false. */
bool
list_empty (struct list *list) {
	return list_begin (list) == list_end (list);
}

/* A와 B가 가리키는 `struct list_elem *'를 서로 바꿈. */
static void
swap (struct list_elem **a, struct list_elem **b) {
	struct list_elem *t = *a;
	*a = *b;
	*b = t;
}

/* LIST의 순서를 뒤집음. */
void
list_reverse (struct list *list) {
	if (!list_empty (list)) {
		struct list_elem *e;

		for (e = list_begin (list); e != list_end (list); e = e->prev)
			swap (&e->prev, &e->next);
		swap (&list->head.next, &list->tail.prev);
		swap (&list->head.next->prev, &list->tail.prev->next);
	}
}

/* 리스트 원소 A부터 B 직전까지가 보조 데이터 AUX를 받는 LESS
   기준으로 정렬돼 있을 때만 true를 돌려줌. */
static bool
is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	if (a != b)
		while ((a = list_next (a)) != b)
			if (less (a, list_prev (a), aux))
				return false;
	return true;
}

/* A에서 시작해 B를 넘지 않는 범위에서, 보조 데이터 AUX를 받는
   LESS 기준으로 감소하지 않는 순서로 이어진 원소 구간(run)을 찾음.
   그 구간의 끝(포함하지 않음)을 돌려줌.
   A부터 B 직전까지는 비어 있지 않은 범위여야 함. */
static struct list_elem *
find_end_of_run (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	ASSERT (a != NULL);
	ASSERT (b != NULL);
	ASSERT (less != NULL);
	ASSERT (a != b);

	do {
		a = list_next (a);
	} while (a != b && !less (a, list_prev (a), aux));
	return a;
}

/* A0부터 A1B0 직전까지와 A1B0부터 B1 직전까지를 합쳐서, 역시
   B1 직전에서 끝나는 하나의 범위로 만듦. 두 입력 범위는 비어 있지
   않아야 하고, 보조 데이터 AUX를 받는 LESS 기준으로 감소하지 않는
   순서로 정렬돼 있어야 함. 결과 범위도 같은 방식으로 정렬됨. */
static void
inplace_merge (struct list_elem *a0, struct list_elem *a1b0,
		struct list_elem *b1,
		list_less_func *less, void *aux) {
	ASSERT (a0 != NULL);
	ASSERT (a1b0 != NULL);
	ASSERT (b1 != NULL);
	ASSERT (less != NULL);
	ASSERT (is_sorted (a0, a1b0, less, aux));
	ASSERT (is_sorted (a1b0, b1, less, aux));

	while (a0 != a1b0 && a1b0 != b1)
		if (!less (a1b0, a0, aux))
			a0 = list_next (a0);
		else {
			a1b0 = list_next (a1b0);
			list_splice (a0, list_prev (a1b0), a1b0);
		}
}

/* 보조 데이터 AUX를 받는 LESS 기준으로 LIST를 정렬함. 자연
   반복 병합 정렬을 쓰며, LIST의 원소 수 기준으로 O(n lg n) 시간과
   O(1) 공간이 걸림. */
void
list_sort (struct list *list, list_less_func *less, void *aux) {
	size_t output_run_cnt;        /* 이번 단계에서 만든 구간(run) 수. */

	ASSERT (list != NULL);
	ASSERT (less != NULL);

	/* 감소하지 않는 원소들의 인접 구간을 합치는 과정을 구간이
	   하나만 남을 때까지 리스트 전체에 반복함. */
	do {
		struct list_elem *a0;     /* 첫 번째 구간의 시작. */
		struct list_elem *a1b0;   /* 첫 번째 구간의 끝이자 두 번째 구간의 시작. */
		struct list_elem *b1;     /* 두 번째 구간의 끝. */

		output_run_cnt = 0;
		for (a0 = list_begin (list); a0 != list_end (list); a0 = b1) {
			/* 반복할 때마다 결과 구간이 하나씩 만들어짐. */
			output_run_cnt++;

			/* 감소하지 않는 원소들의 인접 구간 두 개
			   A0...A1B0과 A1B0...B1을 찾음. */
			a1b0 = find_end_of_run (a0, list_end (list), less, aux);
			if (a1b0 == list_end (list))
				break;
			b1 = find_end_of_run (a1b0, list_end (list), less, aux);

			/* 두 구간을 합침. */
			inplace_merge (a0, a1b0, b1, less, aux);
		}
	}
	while (output_run_cnt > 1);

	ASSERT (is_sorted (list_begin (list), list_end (list), less, aux));
}

/* ELEM을 LIST의 알맞은 위치에 넣음. LIST는 보조 데이터 AUX를
   받는 LESS 기준으로 정렬돼 있어야 함.
   LIST의 원소 수 기준으로 평균 O(n) 시간이 걸림. */
void
list_insert_ordered (struct list *list, struct list_elem *elem,
		list_less_func *less, void *aux) {
	struct list_elem *e;

	ASSERT (list != NULL);
	ASSERT (elem != NULL);
	ASSERT (less != NULL);

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		if (less (elem, e, aux))
			break;
	return list_insert (e, elem);
}

/* LIST를 순회하면서, 보조 데이터 AUX를 받는 LESS 기준으로
   같은 값인 인접 원소 묶음마다 첫 원소만 남기고 나머지를 지움.
   DUPLICATES가 널이 아니면 지운 원소들을 DUPLICATES 뒤에 붙임. */
void
list_unique (struct list *list, struct list *duplicates,
		list_less_func *less, void *aux) {
	struct list_elem *elem, *next;

	ASSERT (list != NULL);
	ASSERT (less != NULL);
	if (list_empty (list))
		return;

	elem = list_begin (list);
	while ((next = list_next (elem)) != list_end (list))
		if (!less (elem, next, aux) && !less (next, elem, aux)) {
			list_remove (next);
			if (duplicates != NULL)
				list_push_back (duplicates, next);
		} else
			elem = next;
}

/* 보조 데이터 AUX를 받는 LESS 기준으로 LIST에서 가장 큰 원소를
   돌려줌. 최댓값이 여러 개면 리스트에서 더 앞에 있는 것을 돌려줌.
   리스트가 비어 있으면 tail을 돌려줌. */
struct list_elem *
list_max (struct list *list, list_less_func *less, void *aux) {
	struct list_elem *max = list_begin (list);
	if (max != list_end (list)) {
		struct list_elem *e;

		for (e = list_next (max); e != list_end (list); e = list_next (e))
			if (less (max, e, aux))
				max = e;
	}
	return max;
}

/* 보조 데이터 AUX를 받는 LESS 기준으로 LIST에서 가장 작은
   원소를 돌려줌. 최솟값이 여러 개면 리스트에서 더 앞에 있는 것을
   돌려줌. 리스트가 비어 있으면 tail을 돌려줌. */
struct list_elem *
list_min (struct list *list, list_less_func *less, void *aux) {
	struct list_elem *min = list_begin (list);
	if (min != list_end (list)) {
		struct list_elem *e;

		for (e = list_next (min); e != list_end (list); e = list_next (e))
			if (less (e, min, aux))
				min = e;
	}
	return min;
}
