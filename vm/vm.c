/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
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

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */

// 가상메모리 페이지를 동적으로 할당하고 초기화한다.
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT) //anonymous page면 안되니까

	struct supplemental_page_table *spt = &thread_current ()->spt; 
	// spt는 현재 실행중인 스레드의 보조페이지테이블을 가리킨다.
	// 스레드 구조체에 spt 멤버가 있나?
	//  spt포인터를 사용하여 현재 실행중인 스레드의 보조 페이지 테이블에 접근하고 조작할 수 있다.

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {

		struct page* page = (struct page*)malloc(sizeof(struct page));

		bool (*initializer)(struct page *, enum vm_type, void *);

		if (VM_TYPE(type) == VM_ANON){
			initializer = anon_initializer;
		}
		else if(VM_TYPE(type)==VM_FILE){

			initializer = file_backed_initializer;
		}

		uninit_new(page, upage, init, type, aux, initializer);

		page->writable = writable;

		spt_insert_page(spt, page);
		//&spt->hash는 spt->hash 멤버의 주소를 반환합니다.

				
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) 
{
	struct page *page = NULL;
	/* TODO: Fill this function. */
	page = (struct page*)malloc(sizeof(struct page));
	struct hash_elem *e;

	//va에 해당하는 hash_elem 찾기
	page->va = pg_round_down(va);
	e = hash_find(&spt, &page->hash_elem);
	free(page);

	//있으면 e에 해당하는 페이지 반환
	return e != NULL ? hash_entry(e, struct page, hash_elem): NULL;

}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;

	/* TODO: Fill this function. */

	return page_insert(&spt->spt_hash, page); //존재하지 않을 경우에만 삽입
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/

// 물리메모리에서 사용가능한 페이지(frame)을 가져오는 함수
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	void *kva = palloc_get_page(PAL_USER); // user pool에서 새로운 physical page를 가져온다.

	if(kva == NULL) // page 할당 실패 -> 나중에 swap_out 처리
		PANIC("todo"); // os를 중지시키고, 소스 파일명, 라인 번호, 함수명 등의 정보와 함께 사용자 지정 메세지를 출력

	frame = malloc(sizeof(struct frame)); // 프레임 할당
	frame -> kva  = kva; // 프레임 멤버 초기화

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */

	return swap_in (page, frame->kva);
}

/* Returns a hash value for page p. */
/* page_hash 함수는 struct page 구조체의 va 멤버 변수를 해시 함수(hash_bytes)에 입력으로 전달하여, 
해당 멤버 변수를 해시 값으로 변환합니다. 
이러한 해시 함수를 사용하여, struct page 구조체를 해시 테이블에 저장할 수 있습니다*/
unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
    const struct page *p = hash_entry(p_, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}

// page a가 page b를 앞서면 return true
	bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
    const struct page *a = hash_entry(a_, struct page, hash_elem);
    const struct page *b = hash_entry(b_, struct page, hash_elem);

    return a->va < b->va;
}

/* Returns true if page a precedes page b. */
bool 
page_less(const struct hash_elem *a_,
               const struct hash_elem *b_, void *aux UNUSED)
{
    const struct page *a = hash_entry(a_, struct page, hash_elem);
    const struct page *b = hash_entry(b_, struct page, hash_elem);

    return a->va < b->va;
}

// page_insert 함수는 페이지(Page)를 해시 테이블(Hash Table)에 삽입하고, 삽입 결과를 반환하는 함수입니다. 
bool
page_insert(struct hash *h, struct page *p){

	if(!hash_insert(h,&p->hash_elem))
		return true;
	else
		return false;

}
// page_delete 함수는 해시 테이블(Hash Table)에서 페이지(Page)를 삭제하고, 삭제 결과를 반환하는 함수입니다.
bool
page_delete(struct hash *h, struct page *p){

	if(!hash_delete(h, &p->hash_elem)){
		return true;
	}
	else
		return false;
}


/* Initialize new supplemental page table */
// spt를 hash table로 구현했기 때문에, hash_init()을 통해 초기화
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {

	hash_init(spt, page_hash, page_less, NULL);

}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
