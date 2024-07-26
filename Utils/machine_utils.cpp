#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <limits.h>
#include "machine_utils.h"


std::string machine_utils::get_host_name()
{
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0) 
        return std::string("invalid host_name");
    return std::string(hostname);
} 

std::string machine_utils::get_mac_address()
{
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strcpy(s.ifr_name, "eth0");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) 
    {
        unsigned char* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(mac_str);
    }
    return std::string();
} 