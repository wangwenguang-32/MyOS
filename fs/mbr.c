#include<stdint.h>
#include<blk_dev.h>
#include<printf.h>
#include<mbr.h>
#include<common.h>

part_t mbr_part[4];


void mbr_init()
{
    blk_cache_node_t*node=blk_read(0);
    mbr_t*mbr=(mbr_t*)node->data;
    if(mbr->boot_signature!=0xAA55U)
    {
        printf("%x\n",mbr->boot_signature);
        return;
    }
    loop(4)
    {
        mbr_part[i].index=i;
        if(mbr->partitions[i].boot_flag==0x80)
        {
            mbr_part[i].bootable=1;
        }
        mbr_part[i].start_lba=mbr->partitions[i].lba_start;
        mbr_part[i].sector_count=mbr->partitions[i].lba_size;
        mbr_part[i].type=mbr->partitions[i].part_type;
        printf("start lba:%d size:%d type:%x\n",mbr_part[i].start_lba,mbr_part[i].sector_count,mbr_part[i].type);

    }

    blk_cache_release(node);

}