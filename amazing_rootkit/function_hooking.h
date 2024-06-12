#define HIJACK_SIZE 12

#define DEBUG_HOOK(fmt, ...)

LIST_HEAD(hooked_syms);

struct sym_hook
{
    void *addr;
    unsigned char o_code[HIJACK_SIZE];
    unsigned char n_code[HIJACK_SIZE];
    struct list_head list;
};

struct ksym
{
    char *name;
    unsigned long addr;
};

struct
{
    unsigned short limit;
    unsigned long base;
} __attribute__ ((packed))idtr;

struct
{
    unsigned short off1;
    unsigned short sel;
    unsigned char none, flags;
    unsigned short off2;
} __attribute__ ((packed))idt;

void hijack_start(void *target, void *new);
void hijack_pause(void *target);
void hijack_resume(void *target);
void hijack_stop(void *target);

extern unsigned long __force_order ;

static inline void wp_cr0(unsigned long val) {

    asm volatile("mov %0,%%cr0":"+r"(val),"+m"(__force_order));

}

inline unsigned long disable_wp ( void )
{
    unsigned long cr0;

    preempt_disable();
    barrier();

    cr0 = read_cr0();
    wp_cr0(cr0 & ~X86_CR0_WP);
    return cr0;
}

inline void restore_wp ( unsigned long cr0 )
{
    wp_cr0(cr0);

    barrier();
    preempt_enable();
}


void hijack_start ( void *target, void *new )
{
    struct sym_hook *sa;
    unsigned char o_code[HIJACK_SIZE], n_code[HIJACK_SIZE];
    unsigned long o_cr0;

    memcpy(n_code, "\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xE0", HIJACK_SIZE);
    *(unsigned long long *)&n_code[2] = (unsigned long long)new;

    memcpy(o_code, target, HIJACK_SIZE);

    o_cr0 = disable_wp();
    memcpy(target, n_code, HIJACK_SIZE);
    restore_wp(o_cr0);

    sa = kmalloc(sizeof(*sa), GFP_KERNEL);
    if ( ! sa )
        return;

    sa->addr = target;
    memcpy(sa->o_code, o_code, HIJACK_SIZE);
    memcpy(sa->n_code, n_code, HIJACK_SIZE);

    list_add(&sa->list, &hooked_syms);
}

void hijack_pause ( void *target )
{
    struct sym_hook *sa;

    list_for_each_entry ( sa, &hooked_syms, list )
    if ( target == sa->addr )
    {
        unsigned long o_cr0 = disable_wp();
        memcpy(target, sa->o_code, HIJACK_SIZE);
        restore_wp(o_cr0);
    }
}

void hijack_resume ( void *target )
{
    struct sym_hook *sa;

    list_for_each_entry ( sa, &hooked_syms, list )
    if ( target == sa->addr )
    {
        unsigned long o_cr0 = disable_wp();
        memcpy(target, sa->n_code, HIJACK_SIZE);
        restore_wp(o_cr0);
    }
}

void hijack_stop ( void *target )
{
    struct sym_hook *sa;

    list_for_each_entry ( sa, &hooked_syms, list )
    if ( target == sa->addr )
    {
        unsigned long o_cr0 = disable_wp();
        memcpy(target, sa->o_code, HIJACK_SIZE);
        restore_wp(o_cr0);

        list_del(&sa->list);
        kfree(sa);
        break;
    }
}
