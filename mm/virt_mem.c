#include <virt_mem.h>
#include <phy_mem.h>
#include <kmalloc.h>
#include <printf.h>
#include <memory.h>
#include <stdint.h>




// 销毁进程内存描述符
void mm_destroy(mm_struct_t *mm) {
    if (mm == NULL) {
        return;
    }
    
    // 遍历并释放所有 VMA
    list_node_t *pos;
    vm_area_t *vma, *n;
    
    list_for_each_entry_safe(vma, n, &mm->vm_area_list, vm_list) {
        // 取消映射所有页面
        if (vma->vm_start < vma->vm_end) {
            vm_unmap_range(mm, vma->vm_start, vma->vm_end - vma->vm_start);
        }
        // 从链表中移除
        list_del(&vma->vm_list);
        // 释放 VMA 结构
        free_vma(vma);
    }
    
    // 注意：这里不释放页目录，因为页目录可能被其他进程共享
    // 或者由调用者负责释放
}

// 分配一个新的 VMA
vm_area_t *alloc_vma(void) {
    uint32_t addr = kmalloc(sizeof(vm_area_t));
    if (addr == 0) {
        return NULL;
    }
    vm_area_t *vma = (vm_area_t *)addr;
    
    MEM_ZERO(vma, sizeof(vm_area_t));
    INIT_LIST_HEAD(&vma->vm_list);
    
    return vma;
}

// 释放 VMA
void free_vma(vm_area_t *vma) {
    if (vma != NULL) {
        kfree((void *)vma);
    }
}

// 查找包含指定地址的 VMA
vm_area_t *find_vma(mm_struct_t *mm, uint32_t addr) {
    if (mm == NULL) {
        return NULL;
    }
    
    vm_area_t *vma;
    list_for_each_entry(vma, &mm->vm_area_list, vm_list) {
        if (addr >= vma->vm_start && addr < vma->vm_end) {
            return vma;
        }
    }
    
    return NULL;
}

// 查找与指定地址范围重叠的 VMA
vm_area_t *find_vma_intersection(mm_struct_t *mm, uint32_t start, uint32_t end) {
    if (mm == NULL || start >= end) {
        return NULL;
    }

    
    vm_area_t *vma;
    list_for_each_entry(vma, &mm->vm_area_list, vm_list) {
        // 检查是否有重叠
        if (!(end <= vma->vm_start || start >= vma->vm_end)) {
            
            return vma;
        }
    }
    
    return NULL;
}

// 插入 VMA 到进程地址空间（按地址排序）
int insert_vma(mm_struct_t *mm, vm_area_t *vma) {
    if (mm == NULL || vma == NULL) {
        return -1;
    }
    
    // 检查地址范围是否有效
    if (vma->vm_start >= vma->vm_end) {
        return -1;
    }
    
    // 检查是否与现有 VMA 重叠
    if (find_vma_intersection(mm, vma->vm_start, vma->vm_end) != NULL) {
        return -1;
    }
    
    // 找到合适的插入位置（按地址排序）
    vm_area_t *pos;
    list_for_each_entry(pos, &mm->vm_area_list, vm_list) {
        if (vma->vm_end <= pos->vm_start) {
            list_add_tail(&vma->vm_list, &pos->vm_list);
            mm->vma_count++;
            return 0;
        }
    }
    
    // 如果没有找到合适位置，插入到链表末尾
    list_add_tail(&vma->vm_list, &mm->vm_area_list);
    mm->vma_count++;
    
    return 0;
}

// 从进程地址空间移除 VMA
void remove_vma(mm_struct_t *mm, vm_area_t *vma) {
    if (mm == NULL || vma == NULL) {
        return;
    }
    
    list_del(&vma->vm_list);
    if (mm->vma_count > 0) {
        mm->vma_count--;
    }
}

// 合并相邻的 VMA（如果可能）
void merge_vma(mm_struct_t *mm, vm_area_t *vma) {
    if (mm == NULL || vma == NULL) {
        return;
    }
    
    vm_area_t *prev_vma = NULL;
    vm_area_t *next_vma = NULL;
    vm_area_t *pos;
    
    // 查找前一个 VMA
    list_for_each_entry(pos, &mm->vm_area_list, vm_list) {
        if (pos->vm_list.next == &vma->vm_list) {
            prev_vma = pos;
            break;
        }
    }
    
    // 查找后一个 VMA
    if (vma->vm_list.next != &mm->vm_area_list) {
        next_vma = list_entry(vma->vm_list.next, vm_area_t, vm_list);
    }
    
    // 尝试与前一个 VMA 合并
    if (prev_vma != NULL && 
        prev_vma->vm_end == vma->vm_start &&
        prev_vma->vm_flags == vma->vm_flags &&
        prev_vma->vm_type == vma->vm_type) {
        prev_vma->vm_end = vma->vm_end;
        remove_vma(mm, vma);
        free_vma(vma);
        vma = prev_vma;
    }
    
    // 尝试与后一个 VMA 合并
    if (next_vma != NULL && 
        vma->vm_end == next_vma->vm_start &&
        vma->vm_flags == next_vma->vm_flags &&
        vma->vm_type == next_vma->vm_type) {
        vma->vm_end = next_vma->vm_end;
        remove_vma(mm, next_vma);
        free_vma(next_vma);
    }
}

// 在进程地址空间中分配虚拟内存区域
vm_area_t *vm_alloc(mm_struct_t *mm, uint32_t size, uint32_t flags, vma_type_t type) {
    if (mm == NULL || size == 0) {
        return NULL;
    }
    
    // 页对齐
    uint32_t aligned_size = (size + 0xFFF) & ~0xFFF;
    uint32_t page_count = aligned_size >> 12;
    
    // 查找空闲的虚拟地址空间
    uint32_t start_addr = USER_HEAP_START;
    vm_area_t *vma;
    
    // 从堆起始地址开始查找
    if (mm->heap_end > mm->heap_start) {
        start_addr = mm->heap_end;
    }
    
    // 查找足够大的空闲区域
    uint32_t found_start = 0;
    uint32_t found_end = 0;
    
    if (list_empty(&mm->vm_area_list)) {
        // 如果没有 VMA，直接使用起始地址
        found_start = start_addr;
        found_end = found_start + aligned_size;
    } else {
        // 遍历 VMA 链表，查找空隙
        uint32_t prev_end = start_addr;
        list_for_each_entry(vma, &mm->vm_area_list, vm_list) {
            if (vma->vm_start >= prev_end + aligned_size) {
                found_start = prev_end;
                found_end = found_start + aligned_size;
                break;
            }
            prev_end = vma->vm_end;
        }
        
        // 如果还没找到，在最后一个 VMA 之后分配
        if (found_start == 0) {
            vma = list_entry(mm->vm_area_list.prev, vm_area_t, vm_list);
            found_start = (vma->vm_end + 0xFFF) & ~0xFFF;
            found_end = found_start + aligned_size;
        }
    }
    
    // 检查是否超出用户空间
    if (found_end > USER_SPACE_END) {
        return NULL;
    }
    
    // 创建 VMA
    vm_area_t *new_vma = alloc_vma();
    if (new_vma == NULL) {
        return NULL;
    }
    
    new_vma->vm_start = found_start;
    new_vma->vm_end = found_end;
    new_vma->vm_flags = flags;
    new_vma->vm_type = type;
    new_vma->vm_phys_addr = 0;
    new_vma->vm_file_offset = 0;
    
    // 插入 VMA
    if (insert_vma(mm, new_vma) != 0) {
        free_vma(new_vma);
        return NULL;
    }
    
    // 如果是堆分配，更新堆结束地址
    if (type == VMA_TYPE_HEAP) {
        mm->heap_end = found_end;
    }
    
    return new_vma;
}

// 释放虚拟内存区域
int vm_free(mm_struct_t *mm, uint32_t addr, uint32_t size) {
    if (mm == NULL) {
        return -1;
    }
    
    // 页对齐
    uint32_t aligned_size = (size + 0xFFF) & ~0xFFF;
    uint32_t end_addr = addr + aligned_size;
    
    // 查找并释放所有重叠的 VMA
    vm_area_t *vma, *n;
    list_for_each_entry_safe(vma, n, &mm->vm_area_list, vm_list) {
        if (vma->vm_start < end_addr && vma->vm_end > addr) {
            // 取消映射
            uint32_t unmap_start = (vma->vm_start > addr) ? vma->vm_start : addr;
            uint32_t unmap_end = (vma->vm_end < end_addr) ? vma->vm_end : end_addr;
            vm_unmap_range(mm, unmap_start, unmap_end - unmap_start);
            
            // 如果整个 VMA 都在释放范围内，删除 VMA
            if (vma->vm_start >= addr && vma->vm_end <= end_addr) {
                remove_vma(mm, vma);
                free_vma(vma);
            } else if (vma->vm_start >= addr) {
                // 只释放前半部分
                vma->vm_start = end_addr;
            } else if (vma->vm_end <= end_addr) {
                // 只释放后半部分
                vma->vm_end = addr;
            } else {
                // 释放中间部分，需要分割 VMA
                vm_area_t *new_vma = alloc_vma();
                if (new_vma != NULL) {
                    new_vma->vm_start = end_addr;
                    new_vma->vm_end = vma->vm_end;
                    new_vma->vm_flags = vma->vm_flags;
                    new_vma->vm_type = vma->vm_type;
                    new_vma->vm_phys_addr = vma->vm_phys_addr;
                    new_vma->vm_file_offset = vma->vm_file_offset;
                    
                    vma->vm_end = addr;
                    list_add(&new_vma->vm_list, &vma->vm_list);
                    mm->vma_count++;
                }
            }
        }
    }
    
    return 0;
}

// 映射虚拟地址到物理地址
int vm_map_page(mm_struct_t *mm, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    if (mm == NULL || mm->pgd == NULL) {
        return -1;
    }
    
    // 检查地址是否在用户空间
    if (vaddr < USER_SPACE_START || vaddr >= USER_SPACE_END) {
        return -1;
    }
    
    // 页对齐
    vaddr = vaddr & ~0xFFF;
    paddr = paddr & ~0xFFF;
    
    // 设置用户模式标志
    uint32_t page_flags = flags | PAGE_USER;
    
    return map_virtual_to_physical(mm->pgd, vaddr, paddr, page_flags);
}

// 取消映射虚拟地址
int vm_unmap_page(mm_struct_t *mm, uint32_t vaddr) {
    if (mm == NULL || mm->pgd == NULL) {
        return -1;
    }
    
    return unmap_virtual_address(mm->pgd, vaddr);
}

// 映射虚拟地址范围到物理地址范围
int vm_map_range(mm_struct_t *mm, uint32_t vaddr, uint32_t paddr, uint32_t size, uint32_t flags) {
    if (mm == NULL || size == 0) {
        return -1;
    }
    
    // 页对齐
    uint32_t aligned_size = (size + 0xFFF) & ~0xFFF;
    uint32_t page_count = aligned_size >> 12;
    
    // 设置用户模式标志
    uint32_t page_flags = flags | PAGE_USER;
    
    return map_virtual_range_to_physical(mm->pgd, vaddr, paddr, page_count, page_flags);
}

// 取消映射虚拟地址范围
int vm_unmap_range(mm_struct_t *mm, uint32_t vaddr, uint32_t size) {
    if (mm == NULL || size == 0) {
        return -1;
    }
    
    // 页对齐
    uint32_t aligned_size = (size + 0xFFF) & ~0xFFF;
    uint32_t page_count = aligned_size >> 12;
    uint32_t i;
    
    for (i = 0; i < page_count; i++) {
        if (vm_unmap_page(mm, vaddr + (i << 12)) != 0) {
            // 继续取消映射其他页面，即使某个失败
        }
    }
    
    return 0;
}

// 扩展堆（brk 系统调用）
int vm_brk(mm_struct_t *mm, uint32_t new_brk) {
    if (mm == NULL) {
        return -1;
    }
    
    // 页对齐
    new_brk = (new_brk + 0xFFF) & ~0xFFF;
    
    if (new_brk < mm->heap_start || new_brk > USER_SPACE_END) {
        return -1;
    }
    
    if (new_brk == mm->heap_end) {
        return 0;  // 没有变化
    }
    
    if (new_brk > mm->heap_end) {
        // 扩展堆：分配新的物理页面并映射
        uint32_t old_brk = mm->heap_end;
        uint32_t size = new_brk - old_brk;
        uint32_t page_count = size >> 12;
        uint32_t i;
        
        for (i = 0; i < page_count; i++) {
            uint32_t paddr = alloc_page();
            if (paddr == 0) {
                // 分配失败，回滚
                vm_unmap_range(mm, old_brk, mm->heap_end - old_brk);
                mm->heap_end = old_brk;
                return -1;
            }
            
            if (vm_map_page(mm, old_brk + (i << 12), paddr, 
                          PAGE_PRESENT | PAGE_WRITE | VM_READ | VM_WRITE) != 0) {
                free_page(paddr);
                vm_unmap_range(mm, old_brk, mm->heap_end - old_brk);
                mm->heap_end = old_brk;
                return -1;
            }
        }
        
        // 更新或创建堆 VMA
        vm_area_t *heap_vma = find_vma(mm, mm->heap_start);
        if (heap_vma == NULL || heap_vma->vm_type != VMA_TYPE_HEAP) {
            // 创建新的堆 VMA
            heap_vma = alloc_vma();
            if (heap_vma == NULL) {
                vm_unmap_range(mm, old_brk, new_brk - old_brk);
                return -1;
            }
            heap_vma->vm_start = mm->heap_start;
            heap_vma->vm_end = new_brk;
            heap_vma->vm_flags = VM_READ | VM_WRITE;
            heap_vma->vm_type = VMA_TYPE_HEAP;
            insert_vma(mm, heap_vma);
        } else {
            heap_vma->vm_end = new_brk;
        }
        
        mm->heap_end = new_brk;
    } else {
        // 收缩堆：取消映射并释放物理页面
        uint32_t old_brk = mm->heap_end;
        uint32_t size = old_brk - new_brk;
        
        vm_unmap_range(mm, new_brk, size);
        
        // 释放物理页面
        uint32_t page_count = size >> 12;
        uint32_t i;
        for (i = 0; i < page_count; i++) {
            uint32_t vaddr = new_brk + (i << 12);
            uint32_t paddr;
            if (translate_virtual_to_physical(mm->pgd, vaddr, &paddr) == 0) {
                free_page(paddr & ~0xFFF);
            }
        }
        
        // 更新堆 VMA
        vm_area_t *heap_vma = find_vma(mm, mm->heap_start);
        if (heap_vma != NULL && heap_vma->vm_type == VMA_TYPE_HEAP) {
            heap_vma->vm_end = new_brk;
        }
        
        mm->heap_end = new_brk;
    }
    
    return 0;
}

// 获取堆的当前结束地址
uint32_t vm_get_brk(mm_struct_t *mm) {
    if (mm == NULL) {
        return 0;
    }
    return mm->heap_end;
}

// 检查地址是否在进程地址空间内
int vm_check_range(mm_struct_t *mm, uint32_t addr, uint32_t size) {
    if (mm == NULL) {
        return 0;
    }
    
    uint32_t end_addr = addr + size;
    
    // 检查是否在用户空间范围内
    if (addr < USER_SPACE_START || end_addr > USER_SPACE_END) {
        return 0;
    }
    
    // 检查是否在某个 VMA 内
    vm_area_t *vma = find_vma(mm, addr);
    if (vma == NULL) {
        return 0;
    }
    
    // 检查整个范围是否在同一个 VMA 内
    if (end_addr > vma->vm_end) {
        return 0;
    }
    
    return 1;
}

// 复制进程地址空间（fork 时使用）
int mm_copy(mm_struct_t *dst_mm, mm_struct_t *src_mm) {
    if (dst_mm == NULL || src_mm == NULL) {
        return -1;
    }
    
    vm_area_t *src_vma;
    list_for_each_entry(src_vma, &src_mm->vm_area_list, vm_list) {
        vm_area_t *dst_vma = alloc_vma();
        if (dst_vma == NULL) {
            mm_destroy(dst_mm);
            return -1;
        }
        
        dst_vma->vm_start = src_vma->vm_start;
        dst_vma->vm_end = src_vma->vm_end;
        dst_vma->vm_flags = src_vma->vm_flags;
        dst_vma->vm_type = src_vma->vm_type;
        dst_vma->vm_phys_addr = src_vma->vm_phys_addr;
        dst_vma->vm_file_offset = src_vma->vm_file_offset;
        
        if (insert_vma(dst_mm, dst_vma) != 0) {
            free_vma(dst_vma);
            mm_destroy(dst_mm);
            return -1;
        }
        
        if (src_vma->vm_flags & VM_PRIVATE) {
            uint32_t size = src_vma->vm_end - src_vma->vm_start;
            set_virtual_range_readonly(src_mm->pgd,src_vma->vm_start,size);
            copy_page_directory_range(dst_mm->pgd,src_mm->pgd,src_vma->vm_start,size);
        } else {
            uint32_t size = src_vma->vm_end - src_vma->vm_start;
            uint32_t page_count = size >> 12;
            uint32_t i;
            
            for (i = 0; i < page_count; i++) {
                uint32_t src_vaddr = src_vma->vm_start + (i << 12);
                uint32_t src_paddr;
                
                if (translate_virtual_to_physical(src_mm->pgd, src_vaddr, &src_paddr) == 0) {
                    uint32_t dst_vaddr = dst_vma->vm_start + (i << 12);
                    if (vm_map_page(dst_mm, dst_vaddr, src_paddr, 
                                  PAGE_PRESENT | PAGE_WRITE | dst_vma->vm_flags) != 0) {
                        mm_destroy(dst_mm);
                        return -1;
                    }
                }
            }
        }
    }
    
    dst_mm->heap_start = src_mm->heap_start;
    dst_mm->heap_end = src_mm->heap_end;
    dst_mm->stack_start = src_mm->stack_start;
    dst_mm->stack_end = src_mm->stack_end;
    
    return 0;
}

