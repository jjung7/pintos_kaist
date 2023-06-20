/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"
/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
struct list lru_list;
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
	lock_init(&frame_page_table_lock);
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
unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED);
/* Returns true if page a precedes page b. */
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt = &curr->spt;

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
			initializer = anon_initializer;
		else if (VM_TYPE(type) == VM_FILE)
			initializer = file_backed_initializer;
		uninit_new(page, upage, init, type, aux, initializer);
		page->writable = writable;
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page *page = NULL;
	struct hash_elem *h_elem;
	/* TODO: Fill this function. */
	h_elem = hash_find(&spt->pages, &page->hash_elem);
	page = hash_entry(h_elem, struct page, hash_elem);
	if (page)
		return page;
	else
		return NULL;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	int succ = false;
	struct hash_elem *h_elem;
	h_elem = hash_insert(&spt->pages, &page->hash_elem);
	if (h_elem == NULL)
		return false;
	/* TODO: Fill this function. */
	return true;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	struct frame *frame;
	struct thread *curr = thread_current();
	/* TODO: The policy for eviction is up to you. */
	struct list_elem *e = list_begin(&lru_list);
	while (victim == NULL)
	{
		lock_acquire(&frame_page_table_lock);
		for (e; e != list_end(&lru_list); e = list_next(e))
		{
			victim = list_entry(e, struct frame, lru_elem);
			if (victim->page == NULL)
			{
				lock_release(&frame_page_table_lock);
				return victim;
			}
			if (pml4_is_accessed(curr->pml4, victim->page->va))
			{
				pml4_set_accessed(curr->pml4, victim->page->va, 0);
			}
			else
			{
				lock_release(&frame_page_table_lock);
				return victim;
			}
		}
	}
	lock_release(&frame_page_table_lock);
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */
	if (victim->page)
		swap_out(victim->page);
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = NULL;
	// struct thread *curr = thread_current();
	/* TODO: Fill this function. */
	void *kva = palloc_get_page(PAL_USER);
	if (kva == NULL)
	{
		frame = vm_evict_frame();
		frame->page = NULL;
		return frame;
	}
	frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = kva;
	frame->page = NULL;

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{

	// 1megabyte 보다 아래있으면 된다
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt UNUSED = &curr->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	if (!not_present) // not valid address
		return false;
	if (is_kernel_vaddr(addr) && user)
	{
		// user tries to access kernal addr
		return false;
	}
	void *rsp = f->rsp;
	if (!user)
		rsp = curr->rsp;
	if (USER_STACK - (1 << 20) <= (rsp - 8) && rsp >= addr)
		vm_stack_growth(addr);
	page = spt_find_page(spt, addr);
	if (page == NULL)
		return false;
	if (write == true && page->writable == false)
		return false;
	return vm_do_claim_page(page);
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
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->pages, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}
/* Returns true if page a precedes page b. */
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED)
{
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}
// void vm_free_frame(struct frame *frame)
// {
// 	list_remove(&frame->lru_elem);
// 	free(frame);
// }
