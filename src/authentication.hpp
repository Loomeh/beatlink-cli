#pragma once
#include <string>

#include "TokenResponse.hpp"

class Authentication {
    public:
        void GetToken();
        void WriteTokenFile(const std::string& jsonResponse);
        TokenResponse ReadTokenFile();
};
