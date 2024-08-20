#include "../Agent/BaseModule.h"
#include <minizip/zip.h>

class Screenshot : public BaseModule
{
private:
    void module_impl() override;
    std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
    bool save_png_to_memory(std::vector<unsigned char>& out, int w, int h, std::vector<unsigned char>& data);
    public:void x11_screenshot(std::vector<unsigned char>* data, int& width, int& height);

    Screenshot() = delete;
    Screenshot(const Screenshot &) = delete;

    Screenshot(std::string mtype, const json & module_args) : BaseModule(mtype, module_args) 
    {
        std::cout << module_args.dump() << std::endl;
    };

    ~Screenshot();
};