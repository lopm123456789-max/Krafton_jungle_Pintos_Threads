/* vm.c: 가상 메모리 객체의 일반 인터페이스. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* 각 서브시스템의 초기화 코드를 불러서 가상 메모리 서브시스템을
 * 초기화함. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* Project 4용 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* 위쪽 줄은 수정하지 말 것. */
	/* TODO: 여기에 코드를 작성할 것. */
}

/* 페이지의 타입을 얻음. 페이지가 초기화된 뒤의 타입을 알고 싶을 때
 * 유용함.
 * 이 함수는 이미 완전히 구현돼 있음. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* 도우미 함수들 */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* 초기화 함수와 함께 대기 중인 페이지 객체를 만듦. 페이지를
 * 만들고 싶으면 직접 만들지 말고 이 함수나 `vm_alloc_page`를 거쳐서
 * 만들 것. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* upage가 이미 쓰이고 있는지 확인함. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: 페이지를 만들고, VM 타입에 맞는 초기화 함수를 가져온 뒤,
		 * TODO: uninit_new를 불러서 "uninit" 페이지 구조체를 만들 것.
		 * TODO: uninit_new를 부른 뒤에 필드를 수정해야 함. */

		/* TODO: 페이지를 spt에 넣을 것. */
	}
err:
	return false;
}

/* spt에서 VA를 찾아 페이지를 돌려줌. 오류가 나면 NULL을 돌려줌. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: 이 함수를 채울 것. */

	return page;
}

/* 검증을 거쳐 PAGE를 spt에 넣음. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: 이 함수를 채울 것. */

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* 쫓아낼(evict) struct frame을 얻음. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: 쫓아내기 정책은 자유롭게 정하면 됨. */

	return victim;
}

/* 페이지 하나를 쫓아내고 해당 프레임을 돌려줌.
 * 오류가 나면 NULL을 돌려줌.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: 희생 페이지를 스왑 아웃하고 쫓아낸 프레임을 돌려줄 것. */

	return NULL;
}

/* palloc()으로 프레임을 얻음. 쓸 수 있는 페이지가 없으면 페이지를
 * 쫓아내고 그걸 돌려줌. 항상 올바른 주소를 돌려줌. 즉 사용자 pool
 * 메모리가 꽉 차면 이 함수가 프레임을 쫓아내서 쓸 수 있는 메모리
 * 공간을 확보함.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: 이 함수를 채울 것. */

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* 스택 늘리기. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* 쓰기 보호된 페이지에서 난 폴트 처리 */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* 성공하면 true를 돌려줌 */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: 폴트를 검증할 것 */
	/* TODO: 여기에 코드를 작성할 것 */

	return vm_do_claim_page (page);
}

/* 페이지를 해제함.
 * 이 함수는 수정하지 말 것. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* VA에 할당된 페이지를 확보(claim)함. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: 이 함수를 채울 것 */

	return vm_do_claim_page (page);
}

/* PAGE를 확보하고 mmu를 설정함. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* 연결 설정 */
	frame->page = page;
	page->frame = frame;

	/* TODO: 페이지의 VA를 프레임의 PA로 매핑하는 페이지 테이블
	   * TODO: 엔트리를 넣을 것. */

	return swap_in (page, frame->kva);
}

/* 새 보조 페이지 테이블 초기화 */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
}

/* src의 보조 페이지 테이블을 dst로 복사 */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* 보조 페이지 테이블이 가진 자원을 해제함 */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: 스레드가 가진 supplemental_page_table을 모두 없애고,
	 * TODO: 수정된 내용을 모두 저장소에 다시 쓸 것. */
}
