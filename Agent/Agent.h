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
    std::vector<std::unique_ptr<Task>> tasks;
    json config;

    void run_all_tasks()
    {
        /**/
    }

    void start_from_config(const json & config)
    {
        /*  iterate config
            construct each module derived class 
            build a task
            run all tasks
        */
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
    Agent(){

    }

    ~Agent()
    {
        
    }

    void start();

    void stop();
};
