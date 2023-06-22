/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
/* anon.c: 디스크 이미지가 아닌 페이지 (익명 페이지)의 구현입니다. */

#include "vm/vm.h"
#include "devices/disk.h"

#include "threads/mmu.h"
#include "lib/kernel/bitmap.h"

#define SLOT 8

struct bitmap *swap_bitmap;
size_t swap_slots;
disk_sector_t sectors;

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
/* 익명 페이지의 데이터를 초기화하세요. */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	/* 할 일: swap_disk를 설정하세요. */
	// disk_get(chan_no, dev_no) chan_no = 채널 넘버,장치 넘버
	// 페이지 사이즈: 4KB, 섹터 사이즈: 512 B: 8 개의 섹터 = 1 Swap slot
	// 따라서 스왑 디스크의 섹터 개수 / 8 = Swap slot 개수
	swap_disk = disk_get(1, 1);
	sectors = disk_size(swap_disk);
	swap_bitmap = bitmap_create(sectors);
}

/* Initialize the file mapping */
/* 파일 매핑을 초기화하세요. */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	/* 핸들러를 설정하세요. */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->ofs = -1;

	return true;
}

/* Swap in the page by read contents from the swap disk. */
/* 스왑 디스크에서 내용을 읽어 페이지를 스왑 인하세요. */
/*
디스크에서 메모리로 데이터 내용을 읽어서
스왑 디스크에서 익명 페이지로 스왑합니다.
데이터의 위치는 페이지가 스왑 아웃될 때
페이지 구조에 스왑 디스크가 저장되어 있어야 한다는 것입니다.
스왑 테이블을 업데이트해야 합니다
*/
static bool
anon_swap_in(struct page *page, void *kva)
{
	struct anon_page *anon_page = &page->anon;
	size_t offset = anon_page->ofs;

	if (bitmap_test(swap_bitmap, offset) == 0)
	{
		PANIC("스왑디스크에 없음. 따라서 swap in 못함!");
	}

	for (int i = 0; i < SLOT; ++i)
	{
		disk_read(swap_disk, offset * SLOT + i, kva + (i * DISK_SECTOR_SIZE));
	}
	bitmap_flip(swap_bitmap, offset);
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
/* 페이지의 내용을 스왑 디스크에 쓰고 페이지를 스왑 아웃하세요. */
/*
메모리에서 디스크로 내용을 복사하여 익명 페이지를 스왑 디스크로 교체합니다.
먼저 스왑 테이블을 사용하여 디스크에서 사용 가능한 스왑 슬롯을 찾은
다음 데이터 페이지를 슬롯에 복사합니다.
데이터의 위치는 페이지 구조체에 저장되어야 합니다.
디스크에 사용 가능한 슬롯이 더 이상 없으면 커널 패닉이 발생할 수 있습니다.
*/
static bool
anon_swap_out(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
	uint64_t *pml4 = thread_current()->pml4;
	void *buff = page->frame->kva;
	size_t offset;

	if ((offset = bitmap_scan_and_flip(swap_bitmap, 0, 1, 0)) == BITMAP_ERROR)
	{
		PANIC("bitmap error");
	}

	// 512 바이트 단위로 끊어서 작성? 8 번
	for (int i = 0; i < SLOT; ++i)
	{
		disk_write(swap_disk, offset * SLOT + i, buff + (i * DISK_SECTOR_SIZE));
	}

	anon_page->ofs = offset;
	page->frame = NULL;
	pml4_clear_page(pml4, page->va);
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
/* 익명 페이지를 파괴하세요. 페이지는 호출자에 의해 해제될 것입니다. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
	struct frame *frame = page->frame;
	if (frame != NULL) // frame 해제
	{
		vm_free_frame(frame);
	}

	if (anon_page->ofs != -1)
		bitmap_set(swap_bitmap, anon_page->ofs, 0);
}