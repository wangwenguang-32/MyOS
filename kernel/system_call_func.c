typedef  int(*func_ptr)(void);

extern int sys_fork();

func_ptr system_func_table[]={0x0,0x0,sys_fork};