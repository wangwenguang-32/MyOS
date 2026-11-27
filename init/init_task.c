#include <process.h>
#include <physical_memory.h>
#include <heap.h>
#include <kernel.h>
#include <printf.h>

#define INIT_ENTRY 0x08048000U
#define INIT_STACK_PAGE 0xbffff000U
extern const uint8_t user_init_start[], user_init_end[];
extern void enter_user_init(void);

int prepare_user_init(task_t *task)
{
    uint32_t directory = 0, code = 0, stack = 0, i;
    uint32_t length = user_init_end - user_init_start;
    mm_struct_t *mm = (void *)kzalloc(sizeof(*mm));
    if (!mm || !length || length > PAGE_SIZE) { kfree(mm); return -1; }
    directory = alloc_page(); code = alloc_page(); stack = alloc_page();
    if (!directory || !code || !stack) goto failed;
    mm->pgd = (void *)(directory + PAGE_OFFSET);
    for (i = 0; i < PAGE_DIR_ENTRIES; i++) mm->pgd->entries[i] = 0;
    for (i = PAGE_OFFSET >> 22; i < PAGE_DIR_ENTRIES; i++)
        mm->pgd->entries[i] = _pdt->entries[i];
    for (i = 0; i < PAGE_SIZE; i++) {
        ((uint8_t *)(code + PAGE_OFFSET))[i] = i < length ? user_init_start[i] : 0;
        ((uint8_t *)(stack + PAGE_OFFSET))[i] = 0;
    }
    if (map_virtual_to_physical(mm->pgd, INIT_ENTRY, code, PAGE_PRESENT | PAGE_USER) ||
        map_virtual_to_physical(mm->pgd, INIT_STACK_PAGE, stack,
                                PAGE_PRESENT | PAGE_WRITE | PAGE_USER)) goto failed_tables;
    INIT_LIST_HEAD(&mm->vm_area_list);
    mm->code_start = INIT_ENTRY; mm->code_end = INIT_ENTRY + length;
    mm->stack_start = INIT_STACK_PAGE; mm->stack_end = INIT_STACK_PAGE + PAGE_SIZE;
    task->mm = mm;
    return 0;
failed_tables:
    for (i = 0; i < (PAGE_OFFSET >> 22); i++)
        if (mm->pgd->entries[i] & PAGE_PRESENT) free_page(mm->pgd->entries[i] & ~4095U);
failed:
    if (directory) free_page(directory);
    if (code) free_page(code);
    if (stack) free_page(stack);
    kfree(mm);
    return -1;
}

void init_main(void *argument)
{
    (void)argument;
    printf("[task] Entering user init, pid=%d\n", current->pid);
    enter_user_init();
    for (;;) __asm__ volatile("cli; hlt");
}
