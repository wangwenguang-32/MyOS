


void __attribute__((section(".boot.text"))) _init_pgtable(unsigned long*pgd,unsigned long*pte)
{
    int i=0;
    unsigned long  address=0x0;
    *pgd=0x107003;
    for(i;i<1024;i++)
    {
        *(pte+i)=((address&0xFFFFF000)|0x03);
        address+=0x1000;
    }

    address=0x0;

    pgd[(0xC0000000)>>22]=0x108003;
    for(i;i<2048;i++)
    {
        *(pte+i)=((address&0xFFFFF000)|0x03);
        address+=0x1000;
    }

    pgd[(0xC0400000)>>22]=0x109003;

    for(i;i<3072;i++)
    {
        *(pte+i)=((address&0xFFFFF000)|0x03);
        address+=0x1000;
    }

}

