#include "../Agent/BaseModule.h"

class Rootkit : public BaseModule
{
private:
    void module_impl() override;
    void send_err(json ret_json, std::string str);
public:
    Rootkit() = delete;
    Rootkit(const Rootkit &) = delete;

    Rootkit(std::string mtype, const json & module_args) : BaseModule(mtype, module_args) 
    {
        std::cout << module_args.dump() << std::endl;
    };

    ~Rootkit();
};
