#pragma once

#include <string>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/md5.h>
#include <sstream>

class CryptoUtils
{
public:
<<<<<<< HEAD
    static std::string base64_encode(const std::vector<unsigned char>& data) 
    {
        static const char* base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::string result;
        int i = 0, j = 0;
        unsigned char char_array_3[3], char_array_4[4];
        for (size_t idx = 0; idx < data.size(); idx++) {
            char_array_3[i++] = data[idx];
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                for (i = 0; (i < 4); i++)
                    result += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (j = 0; (j < i + 1); j++)
                result += base64_chars[char_array_4[j]];
            while ((i++ < 3))
                result += '=';
        }
        return result;
    }

=======
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57
    static std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
    {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, bytes_to_encode, in_len);
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string output(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);

        return output;
    }

    static std::string md5(const std::string& input)
    {
        unsigned char digest[MD5_DIGEST_LENGTH];

        // Compute the MD5 hash
        MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);

        // Convert the hash to a hexadecimal string
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
            ss << std::setw(2) << static_cast<int>(digest[i]);
        }
        return ss.str();
    }
};

