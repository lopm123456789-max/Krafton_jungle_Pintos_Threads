/* file.c: 메모리에 올린 파일 객체(mmap 객체) 구현. */

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* 이 구조체는 수정하지 말 것 */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* 파일 vm 초기화 함수 */
void
vm_file_init (void) {
}

/* 파일 기반 페이지 초기화 */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 핸들러 설정 */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* 파일에서 내용을 읽어서 페이지를 스왑 인 함. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* 내용을 파일에 다시 써서 페이지를 스왑 아웃 함. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* 파일 기반 페이지를 없앰. PAGE는 호출한 쪽이 해제함. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* mmap 수행 */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* munmap 수행 */
void
do_munmap (void *addr) {
}
