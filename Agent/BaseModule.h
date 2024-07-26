#include <stdio.h>
#include <iostream>
#include <include/nlohmann/json.hpp>
#include <utils/crypto_utils.h>

/*

BaseModule is a pure virtual class. 
Each derived class shall call BaseModule("derived_module_class_type") c'tor,
passing its correspoing arguments. 

*/

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
private:
    json args;
    std::string module_type;
    std::vector<artifact_t> artifacts;
    bool run;

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
        // communicator::post_with_payload(payload);
    }

    std::string encode_artifacts(artifact_t a)
    {
        return crypto_utils::base64_encode(a.artifact, a.length);
    }

public:
    BaseModule(std::string mtype, json module_args) : args(module_args), type(mtype){
        /**/
    }

    ~BaseModule(){
        /**/
    };

    /*  
    Derived class can override this function to avoid running forever (or until stop is received) 
    This function will be considered as the main function of the module - from an outer world perspective. 
    */
   
    virtual void run()
    {
        while (run)
        {
            module_impl(args);
        }
    }
    
    void stop()
    {
        stop = false;
    }
};




