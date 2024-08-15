#pragma once

#include <thread>
#include <iostream>
#include "BaseModule.h"
#include "../Modules/FileGrabber.h"

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
<<<<<<< HEAD
        
=======
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57
    }

    void stop()
    {
        m->stop();
        task.join();
    }

    static Task * BuildTask(const std::string & module_name, const json & args);
};
