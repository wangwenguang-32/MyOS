/* Runs before paging; arguments are physical pointers from boot.S.
 * Three consecutive page tables: identity 0..4 MiB, high mapping 0..8 MiB.
 * Do not hard-code their placement: the linker controls boot section sizes.
 */
void __attribute__((section(".boot.text")))
_init_pgtable(unsigned long *directory, unsigned long *tables)
{
    unsigned long i;
    for (i = 0; i < 1024; i++) directory[i] = 0;
    directory[0] = (unsigned long)tables | 3;
    directory[768] = (unsigned long)(tables + 1024) | 3;
    directory[769] = (unsigned long)(tables + 2048) | 3;
    for (i = 0; i < 1024; i++) {
        tables[i] = (i << 12) | 3;
        tables[1024 + i] = (i << 12) | 3;
        tables[2048 + i] = ((i + 1024) << 12) | 3;
    }
}
