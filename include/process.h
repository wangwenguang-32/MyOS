#ifndef  PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <list.h>
#include <page.h>

#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x23\n\t" \
	"pushl %%eax\n\t" \
	"pushfl\n\t" \
	"pushl $0x1B\n\t" \
	"pushl $1f\n\t" \
	"iret\n" \
	"1:\tmovl $0x23,%%eax\n\t" \
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")



/* 简单双向链表节点（侵入式） */
typedef struct list_node {
	struct list_node *prev;
	struct list_node *next;
} list_node_t;

/* 任务状态 */
typedef enum {
	TASK_UNUSED = 0,
	TASK_NEW,
	TASK_READY,
	TASK_RUNNING,
	TASK_SLEEPING,
	TASK_BLOCKED,
	TASK_STOPPED,
	TASK_ZOMBIE
} task_state_t;

/* 调度策略 */
typedef enum {
	SCHED_NORMAL = 0,   /* 时间片轮转/完全公平等普通策略占位 */
	SCHED_FIFO,
	SCHED_RR,
	SCHED_IDLE
} sched_policy_t;

/* 线程标志（示例） */
enum task_flags {
	TASK_FLAG_KERNEL_THREAD = 1u << 0,
	TASK_FLAG_NEED_RESCHED  = 1u << 1,
	TASK_FLAG_IN_SYSCALL    = 1u << 2,
	TASK_FLAG_TRACED        = 1u << 3,
	TASK_FLAG_FPU_USED      = 1u << 4,
};

typedef struct cpu_context {
	uint32_t eflags;
	uint32_t esp0;
	uint16_t ss0;
} cpu_context_t;


/* 亲和性 */
typedef struct cpu_affinity {
	uint32_t allowed_mask;  /* 简化为32路 CPU mask */
	uint8_t  preferred_cpu; /* 迁移时优选 CPU */
} cpu_affinity_t;

/* 信号（非常简化的占位） */
typedef struct signal_state {
	uint32_t pending;       /* 位图：挂起的信号 */
	uint32_t blocked;       /* 位图：被屏蔽的信号 */
	void    (*handlers[32])(int); /* 处理函数表 */
} signal_state_t;

/* 命名/可观测 */
#define TASK_NAME_LEN 16

struct i387_struct {
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];
};


struct tss_struct {
	long	back_link;	
	long	esp0;
	long	ss0;		
	long	esp1;
	long	ss1;		
	long	esp2;
	long	ss2;		
	long	cr3;
	long	eip;
	long	eflags;
	long	eax,ecx,edx,ebx;
	long	esp;
	long	ebp;
	long	esi;
	long	edi;
	long	es;		
	long	cs;		
	long	ss;		
	long	ds;		
	long	fs;		
	long	gs;		
	long	ldt;		
	long	trace_bitmap;	
	struct i387_struct i387;
};


typedef struct task_struct {
	/* 标识 */
	int32_t        pid;
	int32_t        ppid;              /* 父进程 ID */
	char           comm[TASK_NAME_LEN];

	/* 状态/标志 */
	volatile task_state_t state;
	uint32_t       flags;

    /*page directory*/
    page_directory_t*pdt;

	/* 调度相关 */
	sched_policy_t policy;
	int32_t        static_prio;       /* 静态优先级 */
	int32_t        dynamic_prio;      /* 动态优先级/权重 */
	uint32_t       time_slice;        /* 剩余时间片 */
	uint64_t       vruntime;          /* CFS 类算法的虚拟运行时间占位 */
	uint64_t       deadline_ns;       /* 软实时/EDF 占位 */

	/* 运行队列/各种链表挂接点 */
	list_node_t    run_node;          /* 运行队列节点 */
	list_node_t    all_tasks_node;    /* 全局任务链表节点 */
	list_node_t    sibling_node;      /* 子进程链表节点 */
	list_node_t    wait_node;         /* 等待队列节点 */

	/* 亲和性与负载均衡 */
	cpu_affinity_t affinity;
	uint8_t        on_cpu;            /* 正在哪个 CPU 上运行（逻辑 ID） */
	uint8_t        last_cpu;          /* 上次运行的 CPU */

	/* 上下文/栈 */
	cpu_context_t  ctx;               /* 进出调度器保存/恢复 */

	/* 地址空间/内存管理 */
	//struct mm_struct *mm;             /* 进程地址空间（线程共享） */
	//struct mm_struct *active_mm;      /* 内核线程使用的活跃 mm（通常借用） */
	//struct vm_area  *mmap;            /* VMA 链表/树根（可选） */

	/* 同步/等待 */
	//struct wait_queue *wait_queue;    /* 简化版本等待队列指针 */
	int32_t        exit_code;         /* 退出码/等待返回值 */

	/* 文件/IPC */
	//struct fd_table *files;           /* 文件描述符表 */

	/* 信号 */
	signal_state_t  sig;

	/* 统计/度量 */
	uint64_t        start_time_ns;    /* 创建时间 */
	uint64_t        last_run_start_ns;/* 最近一次开始运行时间戳 */

} task_t;


task_t* current;

#endif