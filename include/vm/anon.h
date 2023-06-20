#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
#include "filesys/off_t.h"
struct page;
enum vm_type;

struct anon_page
{
    off_t ofs;
};

void vm_anon_init(void);
bool anon_initializer(struct page *page, enum vm_type type, void *kva);

#endif
