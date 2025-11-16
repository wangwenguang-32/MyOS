typedef  int(*func_ptr)(void);

//extern int sys_write1();
//extern int sys_write2();
extern int sys_fork();


int sys_write1()
{
    printf("aaa   ");
}

int sys_write2()
{
    printf("bbb   ");
}



func_ptr system_func_table[]={0x0,0x0,sys_fork,sys_write1,sys_write2};