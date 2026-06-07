#include "authentication.hpp"
#include "debug.hpp"
#include "TokenResponse.hpp"
#include "pkce.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "lib/httplib/httplib.h"
#include "lib/json/json.hpp"
#include <filesystem>
#include <limits.h>
#include <unistd.h>

namespace {

std::filesystem::path GetExecutableDirectory() {
    char executablePath[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", executablePath, sizeof(executablePath) - 1);
    if (length > 0) {
        executablePath[length] = '\0';
        return std::filesystem::path(executablePath).parent_path();
    }

    return std::filesystem::current_path();
}

std::filesystem::path GetTokenFilePath() {
    return GetExecutableDirectory() / "token.json";
}

}


void Authentication::GetToken() {
    std::string provider_auth_url = "https://discord.com/oauth2/authorize";
    std::string provider_token_url = "https://discord.com/api/oauth2/token";
    std::string client_id = "";
    std::string scope = "";

    Pkce pkce;
    PkceCode code = pkce.Generate();

    std::string redirect_uri = "http://localhost:14000/";

    if (gDebugEnabled) {
        std::cout << "[debug] Starting OAuth flow\n";
        std::cout << "[debug] redirect_uri=" << redirect_uri << "\n";
        std::cout << "[debug] provider_auth_url=" << provider_auth_url << "\n";
        std::cout << "[debug] provider_token_url=" << provider_token_url << "\n";
    }

    std::string auth_url = provider_auth_url +
                           "?client_id=" + client_id +
                           "&code_challenge=" + code.challenge +
                           "&scope=" + scope +
                           "&redirect_uri=" + redirect_uri +
                           "&code_challenge_method=S256" +
                           "&response_type=code";

    httplib::Server server;
    std::string auth_code;

    // Local server to catch the redirect
    server.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        if (gDebugEnabled) {
            std::cout << "[debug] OAuth callback received at /\n";
        }
        if (req.has_param("code")) {
            auth_code = req.get_param_value("code");
            if (gDebugEnabled) {
                std::cout << "[debug] OAuth code captured\n";
            }
        } else {
            if (gDebugEnabled) {
                std::cout << "[debug] OAuth callback missing code parameter\n";
            }
        }
        res.set_content("Please return to the app.", "text/html");
        server.stop();
    });

    if (gDebugEnabled) {
        std::cout << "[debug] Opening browser for auth flow\n";
    }
    std::thread listener([&] { server.listen("localhost", 14000); });

    // Launch browser
    system(("xdg-open \"" + auth_url + "\"").c_str());

    // Wait for the server to intercept the callback and stop
    if (gDebugEnabled) {
        std::cout << "[debug] Waiting for OAuth callback\n";
    }
    listener.join();

    // Perform the token exchange if we successfully captured a code
    if (!auth_code.empty()) {
        if (gDebugEnabled) {
            std::cout << "[debug] Exchanging OAuth code for token\n";
        }
        // Create an HTTPS client for Discord's token endpoint
        httplib::SSLClient cli("discord.com", 443);
        cli.enable_server_certificate_verification(!gDebugEnabled);
        if (gDebugEnabled) {
            cli.set_error_logger([](const httplib::Error &error, const httplib::Request *request) {
                std::cerr << "[debug] HTTP client error: "
                          << httplib::to_string(error);
                if (request) {
                    std::cerr << " request=" << request->path;
                }
                std::cerr << "\n";
            });
            std::cout << "[debug] Server certificate verification disabled for debugging\n";
        }

        httplib::Params params{
            { "client_id", client_id },
            { "grant_type", "authorization_code" },
            { "code", auth_code },
            { "redirect_uri", redirect_uri },
            { "code_verifier", code.verifier }
        };

        // Send URL-encoded POST request
        auto res = cli.Post("/api/oauth2/token", params);
        if (res) {
            if (gDebugEnabled) {
                std::cout << "[debug] Token endpoint status=" << res->status << "\n";
            }
            if (res->status == 200) {
                if (gDebugEnabled) {
                    std::cout << "[debug] Token exchange succeeded; writing token.json\n";
                }
                WriteTokenFile(res->body);
            } else {
                std::cerr << "OAuth Error: token endpoint returned status " << res->status << "\n";
            }
        } else {
            std::cerr << "OAuth Error: failed to contact token endpoint ("
                      << httplib::to_string(res.error()) << ")";
            if (gDebugEnabled) {
                if (res.ssl_error() != 0) {
                    std::cerr << " ssl_error=" << res.ssl_error();
                }
                if (res.ssl_backend_error() != 0) {
                    std::cerr << " ssl_backend_error=" << res.ssl_backend_error();
                }
            }
            std::cerr << "\n";
        }
    } else {
        std::cerr << "OAuth Error: no authorization code was captured\n";
    }
}

void Authentication::WriteTokenFile(const std::string& jsonResponse) {
    nlohmann::json token_json = nlohmann::json::parse(jsonResponse);
    if (token_json.contains("expires_in")) {
        int expires_in = token_json["expires_in"].get<int>();
        auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in);
        auto expires_at_epoch = std::chrono::duration_cast<std::chrono::seconds>(expires_at.time_since_epoch()).count();
        token_json["expires_at"] = expires_at_epoch;
    }
    const auto token_path = GetTokenFilePath();
    std::ofstream token_file(token_path);
    token_file << token_json.dump(4);
    token_file.close();
}

TokenResponse Authentication::ReadTokenFile() {
    const auto token_path = GetTokenFilePath();
    std::ifstream token_file(token_path);

    if (!token_file.is_open()) {
        if (gDebugEnabled) {
            std::cout << "[debug] token.json not found at " << token_path << "\n";
        }
        return TokenResponse(); // Return an empty/default response
    }

    nlohmann::json token_data;


    try {
        token_file >> token_data;
        if (gDebugEnabled) {
            std::cout << "[debug] token.json loaded from " << token_path << "\n";
        }

        TokenResponse response;
        response.expires_in = 0;
        response.expires_at = std::chrono::system_clock::time_point{};

        // Safely extract values if they exist in the JSON
        if (token_data.contains("access_token")) {
            response.access_token = token_data["access_token"].get<std::string>();
        }
        if (token_data.contains("token_type")) {
            response.token_type = token_data["token_type"].get<std::string>();
        }
        if (token_data.contains("expires_in")) {
            response.expires_in = token_data["expires_in"].get<int>();
        }

        // Load the expiration time point if available
        if (token_data.contains("expires_at")) {
            auto expires_at_epoch = token_data["expires_at"].get<long long>();
            response.expires_at = std::chrono::system_clock::time_point(std::chrono::seconds(expires_at_epoch));
            if (gDebugEnabled) {
                std::cout << "[debug] token expires_at epoch=" << expires_at_epoch << "\n";
            }
        }

        return response;

    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse token.json: " << e.what() << '\n';
        return TokenResponse(); // Return default on parse failure
    }
}
