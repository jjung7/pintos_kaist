#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

// 추가 헤더파일
#include "threads/synch.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "threads/palloc.h"
// #include "include/vm/vm.h"
// #include "include/lib/kernel/console.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

void check_address(void *addr);

// system call 대응 함수
void halt(void);
void exit(int status);
int fork(const char *thread_name, struct intr_frame *f);
int exec(const char *file);
int wait(int pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
static int read(int fd, void *buffer, size_t size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
static void fd_check(int fd);
// static int process_add_file(struct file *f);
// static bool mmap_validate(void *addr, size_t length, off_t offset);
static void munmap(struct intr_frame *f);
static void *mmap(struct intr_frame *f);

static void ptr_check(void *ptr);
static struct file *get_file(int fd);
extern uint64_t stdin_file;
extern uint64_t stdout_file;
/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void)
{
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init(&filesys_lock); // lock 초기화
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f UNUSED)
{
	// TODO: Your implementation goes here.

	int sys_num = f->R.rax; // syscall number
	printf("sysnum: %d\n", sys_num);
	thread_current()->isp = f->rsp;
	switch (sys_num)
	{
	case SYS_HALT:
		halt(); // 0
		break;
	case SYS_EXIT:
		exit(f->R.rdi); // 1
		break;
	case SYS_FORK:
		f->R.rax = fork(f->R.rdi, f); // 2
		break;
	case SYS_EXEC:
		f->R.rax = exec(f->R.rdi); // 3
		break;
	case SYS_WAIT:
		f->R.rax = wait(f->R.rdi); // 4
		break;
	case SYS_CREATE:
		f->R.rax = create(f->R.rdi, f->R.rsi); // 5
		break;
	case SYS_REMOVE:
		f->R.rax = remove(f->R.rdi); // 6
		break;
	case SYS_OPEN:
		f->R.rax = open(f->R.rdi); // 7
		break;
	case SYS_FILESIZE:
		f->R.rax = filesize(f->R.rdi); // 8
		break;
	case SYS_READ:
		f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx); // 9
		break;
	case SYS_WRITE:
		f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx); // 10
		break;
	case SYS_SEEK:
		seek(f->R.rdi, f->R.rsi); // 11
		break;
	case SYS_TELL:
		f->R.rax = tell(f->R.rdi); // 12
		break;
	case SYS_CLOSE:
		close(f->R.rdi); // 13
		break;
	case SYS_MMAP:
		f->R.rax = mmap(f);
		break;
	case SYS_MUNMAP:
		munmap(f);
		break;
		// printf("system call!\n");
		// thread_exit ();
	}
}
/*
 * 주어진 주소가 올바른 주소인지 확인하는 함수
 */
void check_address(void *addr)
{
	if (addr == NULL)
	{
		exit(-1); // 주소가 없을 경우
	}
	if (!is_user_vaddr(addr))
	{ // 유저 영역에 속해있지 않을 경우
		exit(-1);
	}
}

// 운영체제를 중지한다.
void halt(void)
{
	power_off(); // src/include/threads/init.h
}

// 현재 프로세스를 중지한다.
void exit(int status)
{
	struct thread *curr = thread_current();
	curr->exit_status = status;
	printf("%s: exit(%d)\n", curr->name, status); // 종료 메시지 출력
	thread_exit();
}

// 현재 프로세스를 복사한다.
int fork(const char *thread_name, struct intr_frame *f)
{
	return process_fork(thread_name, f);
}

// 현재 프로세스를 전환한다.
int exec(const char *file)
{
	check_address(file);
	char *f_copy = palloc_get_page(0);
	if (f_copy == NULL)
	{
		exit(-1);
	}
	strlcpy(f_copy, file, PGSIZE);

	if (process_exec(f_copy) == -1)
	{
		exit(-1);
	}

	NOT_REACHED();
	return 0;
}

// 자식 프로세스가 끝날 떄까지 기다린다.
int wait(int pid)
{
	return process_wait(pid);
}

// 파일 생성
bool create(const char *file, unsigned initial_size)
{
	bool fret = false;
	lock_acquire(&filesys_lock);
	check_address(file); // 유저 영역의 주소인지 확인
	fret = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return fret;
}

// 파일 삭제
bool remove(const char *file)
{
	bool fret = false;
	lock_acquire(&filesys_lock);
	check_address(file); // 유저 영역의 주소인지 확인
	fret = filesys_remove(file);
	lock_release(&filesys_lock);
	return fret;
}

// 파일 열기
int open(const char *file)
{
	check_address(file);
	// printf("check address successful\n");
	struct file *f = filesys_open(file);
	// printf("open successful?\n");
	if (f == NULL)
	{
		return -1;
	}
	// 파일 디스크립터 생성하기
	int fd = process_add_file(f);
	// printf("fd successfully made %d\n", fd);

	if (fd == -1)
	{
		// printf("fail to close?\n");
		file_close(f);
	}
	return fd;
}

// 파일 크기
int filesize(int fd)
{
	int fret = 0;
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd); // fd를 이용해 파일 가져오기
	if (f == NULL)
	{
		return -1;
	}
	fret = file_length(f);
	lock_release(&filesys_lock);
	return fret;
}

// 파일 읽기
static int read(int fd, void *buffer, size_t size)
{
	// fd_check(fd);
	check_address(buffer);
	// buffer 페이지에 대해서 읽기만 있으면 안된다.
	// #ifdef VM

	// 	// 진짜 중요: spt find page 에서 va 로 찾을 때 va 는 페이지 단위여야 한다.
	// 	// 꼭 명심하도록 하자!!!

	// 		// 페이지가 존재하지만 스택영역 + 그중에서도 sp 보다 더 작다? 그러면 안된다!
	// 	uintptr_t isp = thread_current()->isp;
	// 	if (page->va == pg_round_down(isp) && buffer < isp)
	// 		exit(-1);
	// #endif

	int result = 0;

	if (fd == 0)
	{
		result = input_getc();
	}
	else if (fd == 1)
	{
		return -1;
	}
	else
	{
		struct page *page = spt_find_page(&thread_current()->spt, pg_round_down(buffer));
		if (page == NULL || !page->writable)
		{
			// printf("page = %p, is it writable? %d \n", page, page->writable);
			exit(-1);
		}
		struct file *f = process_get_file(fd);
		if (f == NULL)
		{
			return -1;
		}
		lock_acquire(&filesys_lock);
		printf("asdfasdfadsf\n");
		result = file_read(f, buffer, size);
		lock_release(&filesys_lock);
	}
	return result;
}
// 파일 쓰기
int write(int fd, const void *buffer, unsigned size)
{
	check_address(buffer);
	// printf("fd는 1이여야함 : %d\n", fd);
	int result = 0;

	if (fd == 1)
	{
		putbuf(buffer, size);
		result = size;
		// printf("buffer에 잘들어갔나? %d\n", result);
	}
	else if (fd == 0)
	{
		return -1;
	}
	else
	{
		struct file *f = process_get_file(fd);
		if (f == NULL)
		{
			return -1;
		}
		lock_acquire(&filesys_lock);
		result = file_write(f, buffer, size);
		lock_release(&filesys_lock);
	}
	return result;
}

// 파일 위치 변경
void seek(int fd, unsigned position)
{
	if (fd < 2)
	{ // 예약된 파일은 변경 불가
		return;
	}
	struct file *f = process_get_file(fd);
	if (f == NULL)
	{
		return;
	}
	file_seek(f, position);
}

// 파일 현재 위치 반환
unsigned tell(int fd)
{
	if (fd < 2)
	{ // 예약된 파일은 변경 불가
		return;
	}
	struct file *f = process_get_file(fd);
	if (f == NULL)
	{
		return;
	}
	return file_tell(f);
}

// 파일 닫기
void close(int fd)
{
	if (fd < 2)
	{ // 예약된 파일은 변경 불가
		return;
	}
	struct file *f = process_get_file(fd);
	if (f == NULL)
	{
		return;
	}
	file_close(f);
	process_close_file(fd); // fdt에서 제거하기
}

static void *mmap(struct intr_frame *f)
{
	void *addr = (void *)f->R.rdi;
	size_t length = (size_t)f->R.rsi;
	int writable = f->R.rdx;
	int fd = f->R.r10;
	off_t offset = (off_t)f->R.r8;
	void *succ = NULL;

	struct file *cur_file = get_file(fd);
	/* 실패 할때 NULL 반환 */

	// fd로 열린 파일의 길이가 0 바이트 면 호출 실패 or length 이 0 일때도 실패
	if (file_length(cur_file) <= 0 || (int)length <= 0)
	{
		return succ;
	}

	// addr 이 NULL 인 경우 실패 or addr이 페이지 정렬 안되면 실패
	if (addr == NULL || ((uint64_t)addr % PGSIZE) != 0)
	{
		return succ;
	}

	/* 매핑된 페이지 범위가 실행 가능한 로드시간에 매핑된 스택 또는
	페이지 포함하여 기존 매핑된 페이지 집합과 겹치는 경우 실패 ?? */
	if (!is_user_vaddr(addr) || !is_user_vaddr(addr + length) || spt_find_page(&thread_current()->spt, addr))
	{
		return succ;
	}

	// 콘솔 입력 및 출력 파일 설명자는 매핑 할수 없다
	if (fd == 0 || fd == 1)
	{
		return succ;
	}

	if (offset > length)
	{
		return succ;
	}

	return do_mmap(addr, length, writable, cur_file, offset);
}

static void munmap(struct intr_frame *f)
{
	void *addr = (void *)f->R.rdi;

	ptr_check(addr);

	// addr이 페이지 정렬 안되면 실패
	if ((uint64_t)addr % PGSIZE != 0)
	{
		exit(-1);
	}

	do_munmap(addr);
}

/* 현재 존재하는 파일을 가져올때 파일이 없다면 exit(-1) 함*/
static struct file *get_file(int fd)
{
	struct thread *curr = thread_current();

	if (*(curr->fdt + fd) == NULL)
	{
		exit(-1);
	}

	return *(curr->fdt + fd);
}

static void ptr_check(void *ptr)
{
	if (ptr == NULL || !is_user_vaddr((uint64_t)ptr))
	{
		exit(-1);
	}
}

static void fd_check(int fd)
{
	if (fd < 0 || fd >= 128)
	{
		exit(-1);
	}
}

/*
 * 새로운 파일 객체제 대한 파일 디스크립터 생성하는 함수
 * fdt에도 추가해준다.
 */
int process_add_file(struct file *f)
{
	struct thread *cur = thread_current();
	struct file **fdt = cur->fdt;

	// 범위를 벗어나지 않고 인덱스에 값이 존재하지 않을 때까지
	while (cur->next_fd < 128 && fdt[cur->next_fd])
	{
		cur->next_fd++;
	}

	if (cur->next_fd >= 128)
	{ // 범위를 넘어설 때까지 남은 공간이 없으면
		return -1;
	}
	fdt[cur->next_fd] = f;

	return cur->next_fd;
}