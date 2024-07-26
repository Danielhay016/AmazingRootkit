#include <string>
#include <iostream>

class crypto_utils
{
public:
    static std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
    static std::string md5(const std::string& input);
};

