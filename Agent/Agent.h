#include "BaseModule.h"
#include "Task.h"
#include <vector>
#include <thread>
#include "Communicator/communicator.h"

using namespace std;

class Agent
{
private:
    std::vector<Task> tasks;

    void run()
    {
        /**/
    }

public:
    Agent() : agent_id(generate_agent_id()) {
        /**/
    }

    ~Agent();

    void start()
    {
        json config_from_c2 = communicator::getInstance().c2_registration();
        start_from_config(config_from_c2);
    }

    void start_from_config(const json & config)
    {
        /*  iterate config
            construct each module derived class 
            build a task
            run all tasks
        */
    }

    void stop()
    {
        for (Task t : tasks)
        {
            t.stop();
        }
        
    }
};

Agent::Agent(/* args */)
{
}

Agent::~Agent()
{
}
