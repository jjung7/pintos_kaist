// /* vm.c: Generic interface for virtual memory objects. */

// #include "threads/malloc.h"
// #include "vm/vm.h"
// #include "vm/inspect.h"
// #include "lib/kernel/hash.h"
// #include "include/threads/vaddr.h"
// #include "include/threads/mmu.h"
// /* Initializes the virtual memory subsystem by invoking each subsystem's
//  * intialize codes. */
// struct list lru_list;
// void vm_init(void)
// {
// 	vm_anon_init();
// 	vm_file_init();
// #ifdef EFILESYS /* For project 4 */
// 	pagecache_init();
// #endif
// 	register_inspect_intr();
// 	/* DO NOT MODIFY UPPER LINES. */
// 	/* TODO: Your code goes here. */
// 	list_init(&lru_list);
// }

// /* Get the type of the page. This function is useful if you want to know the
//  * type of the page after it will be initialized.
//  * This function is fully implemented now. */
// enum vm_type
// page_get_type(struct page *page)
// {
// 	int ty = VM_TYPE(page->operations->type);
// 	switch (ty)
// 	{
// 	case VM_UNINIT:
// 		return VM_TYPE(page->uninit.type);
// 	default:
// 		return ty;
// 	}
// }

// /* Helpers */
// static struct frame *vm_get_victim(void);
// static bool vm_do_claim_page(struct page *page);
// static struct frame *vm_evict_frame(void);
// unsigned
// page_hash(const struct hash_elem *p_, void *aux UNUSED);
// /* Returns true if page a precedes page b. */
// bool page_less(const struct hash_elem *a_,
// 			   const struct hash_elem *b_, void *aux UNUSED);

// /* Create the pending page object with initializer. If you want to create a
//  * page, do not create it directly and make it through this function or
//  * `vm_alloc_page`. */
// bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
// 									vm_initializer *init, void *aux)
// {

// 	ASSERT(VM_TYPE(type) != VM_UNINIT)
// 	struct thread *curr = thread_current();
// 	struct supplemental_page_table *spt = &curr->spt;

// 	/* Check wheter the upage is already occupied or not. */
// 	if (spt_find_page(spt, upage) == NULL)
// 	{
// 		/* TODO: Create the page, fetch the initialier according to the VM type,
// 		 * TODO: and then create "uninit" page struct by calling uninit_new. You
// 		 * TODO: should modify the field after calling the uninit_new. */

// 		/* TODO: Insert the page into the spt. */
// 		struct page *page = (struct page *)malloc(sizeof(struct page));
// 		bool (*initializer)(struct page *, enum vm_type, void *);
// 		if (VM_TYPE(type) == VM_ANON)
// 			initializer = anon_initializer;
// 		else if (VM_TYPE(type) == VM_FILE)
// 			initializer = file_backed_initializer;
// 		uninit_new(page, upage, init, type, aux, initializer);
// 		page->writable = writable;
// 		return spt_insert_page(spt, page);
// 	}
// err:
// 	return false;
// }

// /* Find VA from spt and return page. On error, return NULL. */
// struct page *
// spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
// {
// 	struct page *page = NULL;
// 	struct hash_elem *h_elem;
// 	/* TODO: Fill this function. */
// 	h_elem = hash_find(&spt->pages, &page->hash_elem);
// 	page = hash_entry(h_elem, struct page, hash_elem);
// 	if (page)
// 		return page;
// 	else
// 		return NULL;
// }

// /* Insert PAGE into spt with validation. */
// bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
// 					 struct page *page UNUSED)
// {
// 	int succ = false;
// 	struct hash_elem *h_elem;
// 	h_elem = hash_insert(&spt->pages, &page->hash_elem);
// 	if (h_elem == NULL)
// 		return false;
// 	/* TODO: Fill this function. */
// 	return true;
// }

// void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
// {
// 	vm_dealloc_page(page);
// 	return true;
// }

// /* Get the struct frame, that will be evicted. */
// static struct frame *
// vm_get_victim(void)
// {
// 	struct frame *victim = NULL;
// 	struct frame *frame;
// 	struct thread *curr = thread_current();
// 	/* TODO: The policy for eviction is up to you. */
// 	struct list_elem *e = list_begin(&lru_list);
// 	while (victim == NULL)
// 	{
// 		lock_acquire(&frame_page_table_lock);
// 		for (e; e != list_end(&lru_list); e = list_next(e))
// 		{
// 			victim = list_entry(e, struct frame, lru_elem);
// 			if (victim->page == NULL)
// 			{
// 				lock_release(&frame_page_table_lock);
// 				return victim;
// 			}
// 			if (pml4_is_accessed(curr->pml4, victim->page->va))
// 			{
// 				pml4_set_accessed(curr->pml4, victim->page->va, 0);
// 			}
// 			else
// 			{
// 				lock_release(&frame_page_table_lock);
// 				return victim;
// 			}
// 		}
// 	}
// 	lock_release(&frame_page_table_lock);
// 	return victim;
// }

// /* Evict one page and return the corresponding frame.
//  * Return NULL on error.*/
// static struct frame *
// vm_evict_frame(void)
// {
// 	struct frame *victim UNUSED = vm_get_victim();
// 	/* TODO: swap out the victim and return the evicted frame. */
// 	if (victim->page)
// 		swap_out(victim->page);
// 	return victim;
// }

// /* palloc() and get frame. If there is no available page, evict the page
//  * and return it. This always return valid address. That is, if the user pool
//  * memory is full, this function evicts the frame to get the available memory
//  * space.*/
// static struct frame *
// vm_get_frame(void)
// {
// 	struct frame *frame = NULL;
// 	// struct thread *curr = thread_current();
// 	/* TODO: Fill this function. */
// 	void *kva = palloc_get_page(PAL_USER);
// 	if (kva == NULL)
// 	{
// 		frame = vm_evict_frame();
// 		frame->page = NULL;
// 		return frame;
// 	}
// 	frame = (struct frame *)malloc(sizeof(struct frame));
// 	frame->kva = kva;
// 	frame->page = NULL;

// 	ASSERT(frame != NULL);
// 	ASSERT(frame->page == NULL);
// 	return frame;
// }

// /* Growing the stack. */
// static void
// vm_stack_growth(void *addr UNUSED)
// {

// 	// 1megabyte 보다 아래있으면 된다
// }

// /* Handle the fault on write_protected page */
// static bool
// vm_handle_wp(struct page *page UNUSED)
// {
// }

// /* Return true on success */
// bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
// 						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
// {
// 	struct thread *curr = thread_current();
// 	struct supplemental_page_table *spt UNUSED = &curr->spt;
// 	struct page *page = NULL;
// 	/* TODO: Validate the fault */
// 	/* TODO: Your code goes here */
// 	if (!not_present) // not valid address
// 		return false;
// 	if (is_kernel_vaddr(addr) && user)
// 	{
// 		// user tries to access kernal addr
// 		return false;
// 	}
// 	void *rsp = f->rsp;
// 	if (!user)
// 		rsp = curr->rsp;
// 	if (USER_STACK - (1 << 20) <= (rsp - 8) && rsp >= addr)
// 		vm_stack_growth(addr);
// 	page = spt_find_page(spt, addr);
// 	if (page == NULL)
// 		return false;
// 	if (write == true && page->writable == false)
// 		return false;
// 	return vm_do_claim_page(page);
// }

// /* Free the page.
//  * DO NOT MODIFY THIS FUNCTION. */
// void vm_dealloc_page(struct page *page)
// {
// 	destroy(page);
// 	free(page);
// }

// /* Claim the page that allocate on VA. */
// bool vm_claim_page(void *va UNUSED)
// {
// 	struct page *page = NULL;
// 	/* TODO: Fill this function */

// 	return vm_do_claim_page(page);
// }

// /* Claim the PAGE and set up the mmu. */
// static bool
// vm_do_claim_page(struct page *page)
// {
// 	struct frame *frame = vm_get_frame();

// 	/* Set links */
// 	frame->page = page;
// 	page->frame = frame;

// 	/* TODO: Insert page table entry to map page's VA to frame's PA. */

// 	return swap_in(page, frame->kva);
// }

// /* Initialize new supplemental page table */
// void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
// {
// 	hash_init(&spt->pages, page_hash, page_less, NULL);
// }

// /* Copy supplemental page table from src to dst */
// bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
// 								  struct supplemental_page_table *src UNUSED)
// {
// }

// /* Free the resource hold by the supplemental page table */
// void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
// {
// 	/* TODO: Destroy all the supplemental_page_table hold by thread and
// 	 * TODO: writeback all the modified contents to the storage. */
// }

// unsigned
// page_hash(const struct hash_elem *p_, void *aux UNUSED)
// {
// 	const struct page *p = hash_entry(p_, struct page, hash_elem);
// 	return hash_bytes(&p->va, sizeof p->va);
// }
// /* Returns true if page a precedes page b. */
// bool page_less(const struct hash_elem *a_,
// 			   const struct hash_elem *b_, void *aux UNUSED)
// {
// 	const struct page *a = hash_entry(a_, struct page, hash_elem);
// 	const struct page *b = hash_entry(b_, struct page, hash_elem);

// 	return a->va < b->va;
// }
// // void vm_free_frame(struct frame *frame)
// // {
// // 	list_remove(&frame->lru_elem);
// // 	free(frame);
// // }

#include "vm/vm.h"
#include "string.h"
#include "threads/malloc.h"
#include "threads/mmu.h"
#include "userprog/process.h"
#include "vm/inspect.h"
#include "include/threads/vaddr.h"

static struct list lru_list;
static struct lock lru_lock;

static uint64_t page_hash(const struct hash_elem *e, void *aux);
static bool page_less(const struct hash_elem *a,
					  const struct hash_elem *b,
					  void *aux);
static void page_destructor(struct hash_elem *e, void *aux);
bool setup_page_table(void *upage, void *kpage, bool writable);

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&lru_list);
	lock_init(&lru_lock);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
// 									vm_initializer *init, void *aux)
// {
// 	ASSERT(VM_TYPE(type) != VM_UNINIT)
// 	// printf("type: %d, upage: %p\n", type, upage);

// 	struct supplemental_page_table *spt = &thread_current()->spt;

// 	/* Check wheter the upage is already occupied or not. */
// 	if (spt_find_page(spt, upage) == NULL)
// 	{
// 		/* TODO: Create the page, fetch the initialier according to the VM type,
// 		 * TODO: and then create "uninit" page struct by calling uninit_new. You
// 		 * TODO: should modify the field after calling the uninit_new. */

// 		/* TODO: Insert the page into the spt. */
// 		struct page *page = (struct page *)malloc(sizeof(struct page));
// 		if (page == NULL)
// 			goto err;

// 		bool (*page_init)(struct page *, enum vm_type, void *) = (VM_TYPE(type) == VM_ANON ? anon_initializer : file_backed_initializer);
// 		uninit_new(page, upage, init, type, aux, page_init);
// 		page->writable = writable;
// 		if (!spt_insert_page(spt, page))
// 			goto err;
// 	}
// 	else
// 	{
// 		// printf("upage is already existed\n");
// 		goto err;
// 	}

// 	return true;
// err:
// 	return false;
// }
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt = &curr->spt;
	// printf("alloc까지 오나?\n");
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
		struct page *page = (struct page *)malloc(sizeof(struct page));
		bool (*initializer)(struct page *, enum vm_type, void *);
		if (VM_TYPE(type) == VM_ANON)
			uninit_new(page, upage, init, type, aux, anon_initializer);
		else if (VM_TYPE(type) == VM_FILE)
			uninit_new(page, upage, init, type, aux, file_backed_initializer);
		page->writable = writable;
		// printf("is it writeable? or not? %s\n", page->writable);
		return spt_insert_page(spt, page);
	}
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	// printf("spt find page\n");
	struct hash_elem *hm;
	struct page *page;
	struct page tp;
	tp.va = pg_round_down(va);
	/* TODO: Fill this function. */
	lock_acquire(&spt->page_lock);
	hm = hash_find(&spt->pages, &tp.hash_elem);
	page = hash_entry(hm, struct page, hash_elem);
	lock_release(&spt->page_lock);
	if (hm)
	{
		return page;
	}
	else
	{
		return NULL;
	}
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	// printf("spt insert page %p, %d\n", page->va, page_get_type(page));
	struct hash_elem *hm = NULL;
	/* TODO: Fill this function. */
	// checks that the virtual address does not exist in the given spt
	if (spt_find_page(spt, page->va))
	{
		return false;
	}
	lock_acquire(&spt->page_lock);
	hm = hash_insert(&spt->pages, &page->hash_elem);
	lock_release(&spt->page_lock);
	return hm == NULL;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	if (page != NULL)
	{
		vm_dealloc_page(page);
	}
}

/* Get the struct frame, that will be evicted. */
// static struct frame *
// vm_get_victim(void)
// {
// 	struct frame *frame = NULL;
// 	/* TODO: The policy for eviction is up to you. */
// 	struct list_elem *p;
// 	lock_acquire(&lru_lock);

// 	for (p = list_begin(&lru_list); p != list_end(&lru_list); p = list_next(p))
// 	{
// 		frame = list_entry(p, struct frame, lru_elem);
// 		uint64_t *pml4 = thread_current()->pml4;
// 		// find pte
// 		if (frame->page == NULL)
// 		{
// 			break;
// 		}
// 		if (pml4_is_accessed(pml4, frame->page->va))
// 			pml4_set_accessed(pml4, frame->page->va, 0);
// 		else
// 		{
// 			break;
// 		}
// 	}

// 	lock_release(&lru_lock);
// 	return frame;
// }

// /* Evict one page and return the corresponding frame.
//  * Return NULL on error.*/
// static struct frame *
// vm_evict_frame(void)
// {
// 	struct frame *victim = vm_get_victim();
// 	/* TODO: swap out the victim and return the evicted frame. */
// 	if (victim->page != NULL)
// 		swap_out(victim->page);
// 	return victim;
// }

// /* palloc() and get frame. If there is no available page, evict the page
//  * and return it. This always return valid address. That is, if the user pool
//  * memory is full, this function evicts the frame to get the available memory
//  * space.*/

static struct frame *
vm_get_victim(void)
{

	struct frame *victim = NULL;

	/* TODO: The policy for eviction is up to you. */
	struct list_elem *e;
	lock_acquire(&lru_lock);
	for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e))
	{
		struct thread *curr = thread_current();
		victim = list_entry(e, struct frame, lru_elem); // 할당된 페이지가 없을 경우
		if (victim->page == NULL)
		{
			lock_release(&lru_lock);
			return victim;
		}
		else if (pml4_is_accessed(curr->pml4, victim->page->va)) // check if its recently accessed
		{
			pml4_set_accessed(curr->pml4, victim->page->va, 0); // sets the accessed bit
		}
		else
		{
			lock_release(&lru_lock);
			return victim;
		}
	}
	lock_release(&lru_lock);

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{

	struct frame *victim = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */
	if (victim->page != NULL)
		swap_out(victim->page);

	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
// static struct frame *
// vm_get_frame(void)
// {
// 	struct frame *frame = NULL;
// 	void *kva;
// 	/* TODO: Fill this function. */
// 	if ((kva = palloc_get_page(PAL_USER | PAL_ZERO)) == NULL)
// 	{
// 		frame = vm_evict_frame();
// 		memset(frame->kva, 0, PGSIZE);
// 		frame->page = NULL;
// 		frame->thread = thread_current();
// 		return frame;
// 	}

// 	frame = (struct frame *)malloc(sizeof(struct frame));
// 	frame->kva = kva;
// 	frame->page = NULL;
// 	frame->thread = thread_current();

// 	lock_acquire(&lru_lock);
// 	list_push_back(&lru_list, &frame->lru_elem);
// 	lock_release(&lru_lock);

// 	ASSERT(frame != NULL);
// 	ASSERT(frame->kva != NULL);
// 	ASSERT(frame->page == NULL);

// 	return frame;
// }
static struct frame *
vm_get_frame(void)
{

	struct frame *frame = NULL;
	// struct thread *curr = thread_current();
	/* TODO: Fill this function. */
	void *kva = palloc_get_page(PAL_USER | PAL_ZERO);
	if (kva == NULL)
	{
		frame = vm_evict_frame();
		memset(frame->kva, 0, PGSIZE);
		frame->page = NULL;
		return frame;
	}

	frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = kva;
	frame->page = NULL;
	lock_acquire(&lru_lock);
	list_push_back(&lru_list, &frame->lru_elem);
	lock_release(&lru_lock);
	ASSERT(frame != NULL);
	ASSERT(frame->kva != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}
/* Growing the stack. */
// static void
// vm_stack_growth(void *addr)
// {
// 	void *pg_addr = pg_round_down(addr);
// 	ASSERT((uintptr_t)USER_STACK - (uintptr_t)pg_addr <= (1 << 20));

// 	while (vm_alloc_page(VM_ANON, pg_addr, true))
// 	{
// 		struct page *pg = spt_find_page(&thread_current()->spt, pg_addr);
// 		vm_claim_page(pg_addr);
// 		pg_addr += PGSIZE;
// 	}
// }

static void
vm_stack_growth(void *addr UNUSED)
{
	// rsp 가 addr 보다 작아질 때까지
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), true);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

// Page Fault Handler
/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED, bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	if (addr == NULL)
		return false;

	if (is_kernel_vaddr(addr))
		return false;
	void *va = pg_round_down(addr);

	if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
	{
		/* TODO: Validate the fault */
		// todo: 페이지 폴트가 스택 확장에 대한 유효한 경우인지를 확인해야 합니다.
		void *rsp = f->rsp; // user access인 경우 rsp는 유저 stack을 가리킨다.
		if (!user)			// kernel access인 경우 thread에서 rsp를 가져와야 한다.
			rsp = thread_current()->isp;

		// 스택 확장으로 처리할 수 있는 폴트인 경우, vm_stack_growth를 호출
		if ((USER_STACK - (1 << 20) <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK) ||
			(USER_STACK - (1 << 20) <= rsp && rsp <= addr && addr <= USER_STACK))
		{

			vm_stack_growth(addr);
		}

		page = spt_find_page(spt, va);
		if (page == NULL)
			return false;
		if (write && !page->writable) // write 불가능한 페이지에 write 요청한 경우
			return false;
		return vm_do_claim_page(page);
	}
	return false;
}
/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED)
{
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = spt_find_page(spt, va);

	/* TODO: Fill this function */
	if (page == NULL)
		return false;

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{

	struct frame *frame = vm_get_frame();
	bool success = false;

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* TODO: 페이지 테이블 엔트리를 삽입하여
	페이지의 가상 주소(VA)를 프레임의 물리 주소(PA)와 매핑합니다. */
	struct thread *t = thread_current();
	// if (pml4_get_page(t->pml4, page->va) == NULL)
	// { // 물리 주소가 비어있다.
	if (!pml4_set_page(t->pml4, page->va, frame->kva, page->writable))
	{
		// 할당하지 못했다면
		return false;
	}
	success = swap_in(page, frame->kva);
	return success; // uninit_initialize
}
void vm_free_frame(struct frame *frame)
{
	lock_acquire(&lru_lock);
	list_remove(&frame->lru_elem);
	lock_release(&lru_lock);
	free(frame);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->pages, page_hash, page_less, NULL);
	lock_init(&spt->page_lock);
}

// bool supplemental_page_table_copy(struct supplemental_page_table *dst,
// 								  struct supplemental_page_table *src)
// {
// 	struct hash_iterator iter;
// 	hash_first(&iter, &(src->pages));
// 	while (hash_next(&iter))
// 	{
// 		struct page *tmp = hash_entry(hash_cur(&iter), struct page, hash_elem);
// 		struct page *cpy = NULL;
// 		// printf("curr_type: %d, parent_va: %p, aux: %p\n", VM_TYPE(tmp->operations->type), tmp->va, tmp->uninit.aux);

// 		switch (VM_TYPE(tmp->operations->type))
// 		{
// 		case VM_UNINIT:
// 			// printf("tmp->uninit.type: %d, va: %p, aux: %p\n", tmp->uninit.type, tmp->va, tmp->uninit.aux);
// 			if (VM_TYPE(tmp->uninit.type) == VM_ANON)
// 			{
// 				struct file_page *info = (struct file_page *)malloc(sizeof(struct file_page));
// 				memcpy(info, tmp->uninit.aux, sizeof(struct file_page));

// 				info->file = file_duplicate(info->file);

// 				vm_alloc_page_with_initializer(tmp->uninit.type, tmp->va, tmp->writable, tmp->uninit.init, (void *)info);
// 			}
// 			break;
// 		case VM_ANON:
// 			// printf("VMANON\n");
// 			vm_alloc_page(tmp->operations->type, tmp->va, tmp->writable);
// 			cpy = spt_find_page(dst, tmp->va);

// 			// printf("child va : %p, type: %d\n", cpy->va, cpy->operations->type);

// 			if (cpy == NULL)
// 			{
// 				return false;
// 			}

// 			cpy->writable = tmp->writable;
// 			struct frame *cpy_frame = malloc(sizeof(struct frame));
// 			cpy->frame = cpy_frame;
// 			cpy_frame->page = cpy;
// 			// memcpy ?
// 			cpy_frame->kva = tmp->frame->kva;

// 			struct thread *t = thread_current();
// 			lock_acquire(&lru_lock);
// 			list_push_back(&lru_list, &cpy_frame->lru_elem);
// 			lock_release(&lru_lock);

// 			if (pml4_set_page(t->pml4, cpy->va, cpy_frame->kva, 0) == false)
// 			{
// 				// printf("child set page flase \n");
// 				return false;
// 			}
// 			swap_in(cpy, cpy_frame->kva);
// 			break;
// 		case VM_FILE:
// 			break;
// 		default:
// 			break;
// 		}
// 	}
// 	return true;
// }
/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	// TODO: 보조 페이지 테이블을 src에서 dst로 복사합니다.
	// TODO: src의 각 페이지를 순회하고 dst에 해당 entry의 사본을 만듭니다.
	// TODO: uninit page를 할당하고 그것을 즉시 claim해야 합니다.
	struct hash_iterator i;
	lock_acquire(&src->page_lock);
	hash_first(&i, &src->pages);
	while (hash_next(&i))
	{
		// src_page 정보
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		struct page *dst_page = NULL;
		enum vm_type type = src_page->operations->type;
		void *aux = NULL;

		switch (VM_TYPE(type))
		{
		case VM_UNINIT:
			// uninit 타입은 본래 타입으로 할당해야한다?
			aux = src_page->uninit.aux;
			type = page_get_type(src_page);
			struct file_page *fp = NULL;

			if (aux != NULL)
			{
				struct file_page *fd = (struct file_page *)aux;
				fp = (struct file_page *)malloc(sizeof(struct file_page));
				if (VM_TYPE(type) == VM_FILE)
				{
					fp->file = file_reopen(fd->file);
				}
				else
				{
					fp->file = fd->file;
				}
				fp->ofs = fd->ofs;
				fp->read_bytes = fd->read_bytes;
				fp->zero_bytes = fd->zero_bytes;
			}

			vm_alloc_page_with_initializer(type,
										   src_page->va, src_page->writable, src_page->uninit.init, fp);
			break;
		case VM_ANON:
			if (!vm_alloc_page(type, src_page->va, src_page->writable))
			{
				lock_release(&src->page_lock);
				return false;
			}

			if (!vm_claim_page(src_page->va))
			{
				lock_release(&src->page_lock);
				return false;
			}

			// 매핑된 프레임에 내용 로딩
			dst_page = spt_find_page(dst, src_page->va);
			memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
			break;
		case VM_FILE:
			break;
		}
	}
	lock_release(&src->page_lock);
	return true;
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->pages, page_destructor);
}

bool setup_page_table(void *upage, void *kpage, bool writable)
{
	struct thread *t = thread_current();
	return (pml4_get_page(t->pml4, upage) == NULL && pml4_set_page(t->pml4, upage, kpage, writable));
}

static uint64_t page_hash(const struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);
	struct supplemental_page_table *spt = &thread_current()->spt;
	return hash_bytes(&page->va, sizeof page->va);
}
static bool page_less(const struct hash_elem *a,
					  const struct hash_elem *b,
					  void *aux)
{
	struct page *p1 = hash_entry(a, struct page, hash_elem);
	struct page *p2 = hash_entry(b, struct page, hash_elem);
	return p1->va < p2->va;
}

static void page_destructor(struct hash_elem *e, void *aux)
{
	struct page *entry;
	entry = hash_entry(e, struct page, hash_elem);
	vm_dealloc_page(entry);
}