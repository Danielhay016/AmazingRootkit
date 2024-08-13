#pragma once

#include <string>
#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <limits.h>
#include <random>
#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <fstream>


class MachineUtils
{
public:
    MachineUtils() = delete;
    
    static std::string get_host_name()
    {
        char hostname[HOST_NAME_MAX];
        if (gethostname(hostname, sizeof(hostname)) != 0) 
            return std::string("invalid host_name");
        return std::string(hostname);
    }

    static std::string get_mac_address()
    {
        struct ifreq s;
        int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

        strcpy(s.ifr_name, "eth0");
        if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) 
        {
            unsigned char* mac = reinterpret_cast<unsigned char*>(s.ifr_ifru.ifru_hwaddr.sa_data);
            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return std::string(mac_str);
        }
        return std::string();
    }

    static std::string generate_random_string(size_t length)
    {
        const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, characters.size() - 1);

        std::string random_string;
        for (size_t i = 0; i < length; ++i) {
            random_string += characters[distribution(generator)];
        }

        return random_string;
    }


};
