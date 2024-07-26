#include <thread>
#include <iostream>
#include "BaseModule.h"

class Task
{
private:
    std::thread task;
    BaseModule * m; /* m must be declared as pointer because its a pure virtual class */
public:
    Task(BaseModule * module_base) : m(module_base)
    { /**/ };
    ~Task()
    {
        /**/
    };

    void run()
    {
        m->run();
    }

    void stop()
    {
        m->stop();
        task.join();
    }
};
