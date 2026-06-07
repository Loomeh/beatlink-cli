#pragma once
#include <string>

struct PkceCode {
    std::string challenge;
    std::string verifier;
};

class Pkce {
    public:
        PkceCode Generate();
    private:
        std::string Base64UrlEncode(const std::string& input);
};
