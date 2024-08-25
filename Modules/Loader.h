#include "../Agent/BaseModule.h"

#define UK "UNKNOWN"
#define ESCAPE(key) (key == KEY_ESC)
#define SHIFT(key)  ((key == KEY_LEFTSHIFT) || (key == KEY_RIGHTSHIFT))

class MyLoader : public BaseModule
{
private:
    void module_impl() override;
    std::vector<uint8_t> hex_to_bytes(const std::string& hex);

public:
    MyLoader() = delete;
    MyLoader(const MyLoader &) = delete;

    MyLoader(std::string mtype, const json & module_args) : BaseModule(mtype, module_args) 
    {
        std::cout << module_args.dump() << std::endl;
    };

    ~MyLoader();
};
