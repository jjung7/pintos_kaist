/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "include/lib/kernel/bitmap.h"
/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
disk_sector_t sector;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);
struct bitmap *bitmap;
/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	sector = disk_size(swap_disk) / 8;
	bitmap = bitmap_create(sector);
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->ofs = -1;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	ASSERT(page != NULL);

	struct anon_page *anon_page = &page->anon;
	size_t offset = anon_page->ofs;

	if (bitmap_test(bitmap, offset) == 0)
	{
		PANIC("Error! No swap disk");
	}

	for (int i = 0; i < 8; ++i)
	{
		disk_read(swap_disk, offset * 8 + i, kva + (i * DISK_SECTOR_SIZE));
	}
	bitmap_flip(bitmap, offset);
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page)
{
	ASSERT(page != NULL);
	ASSERT(page->frame != NULL);

	struct anon_page *anon_page = &page->anon;
	uint64_t *pml4 = thread_current()->pml4;
	void *buff = page->frame->kva;
	size_t offset;

	if ((offset = bitmap_scan_and_flip(bitmap, 0, 1, 0)) == BITMAP_ERROR)
	{
		PANIC("bitmap error");
	}

	// 512 바이트 단위로 끊어서 작성? 8 번
	for (int i = 0; i < 8; ++i)
	{
		disk_write(swap_disk, offset * 8 + i, buff + (i * DISK_SECTOR_SIZE));
	}

	anon_page->ofs = offset;
	page->frame = NULL;
	pml4_clear_page(pml4, page->va);

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
	struct frame *frame = page->frame;

	if (frame != NULL) // frame 해제
	{
		list_remove(&frame->lru_elem);
		free(frame);
	}

	if (anon_page->ofs != -1)
		bitmap_set(bitmap, anon_page->ofs, 0);
}