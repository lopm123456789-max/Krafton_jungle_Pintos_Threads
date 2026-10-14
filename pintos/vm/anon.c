/* anon.c: 디스크 이미지가 아닌 페이지(익명 페이지) 구현. */

#include "vm/vm.h"
#include "devices/disk.h"

/* 아래 줄은 수정하지 말 것 */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* 이 구조체는 수정하지 말 것 */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* 익명 페이지용 데이터 초기화 */
void
vm_anon_init (void) {
	/* TODO: swap_disk를 설정할 것. */
	swap_disk = NULL;
}

/* 파일 매핑 초기화 */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 핸들러 설정 */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* 스왑 디스크에서 내용을 읽어서 페이지를 스왑 인 함. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* 내용을 스왑 디스크에 써서 페이지를 스왑 아웃 함. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* 익명 페이지를 없앰. PAGE는 호출한 쪽이 해제함. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
