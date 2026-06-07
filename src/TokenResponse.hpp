#pragma once
#include <string>
#include <chrono>

class TokenResponse {
    public:
        std::string access_token;
        std::string token_type;
        int expires_in;
        std::chrono::system_clock::time_point expires_at;

        TokenResponse() = default;
        TokenResponse(const std::string& access_token, const std::string& token_type, int expires_in, std::chrono::system_clock::time_point expires_at);
};
