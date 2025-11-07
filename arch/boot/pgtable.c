


void __attribute__((section(".boot.text"))) _init_pgtable(unsigned long*pgd,unsigned long*pte)
{
    int i=0;
    unsigned long  address=0x0;
    *pgd=0x106007;
    for(i;i<1024;i++)
    {
        *(pte+i)=((address&0xFFFFF000)|0x07);
        address+=0x1000;
    }

    address=0x0;

    pgd[(0xC0000000)>>22]=0x107007;
    for(i;i<2048;i++)
    {
        *(pte+i)=((address&0xFFFFF000)|0x07);
        address+=0x1000;
    }

}

