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

