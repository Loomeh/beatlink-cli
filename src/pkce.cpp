#include "pkce.hpp"
#include <cryptopp/base64.h>
#include <cryptopp/sha.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <array>

PkceCode Pkce::Generate() {
    CryptoPP::AutoSeededRandomPool rng;
    std::array<CryptoPP::byte, 32> buffer{};
    rng.GenerateBlock(buffer.data(), buffer.size());
    std::string verifier = Base64UrlEncode(std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size()));

    CryptoPP::SHA256 hash;
    std::array<CryptoPP::byte, CryptoPP::SHA256::DIGESTSIZE> digest{};
    hash.CalculateDigest(
        digest.data(),
        reinterpret_cast<const CryptoPP::byte*>(verifier.data()),
        verifier.size());
    std::string challenge = Base64UrlEncode(std::string(reinterpret_cast<const char*>(digest.data()), digest.size()));

    return {challenge, verifier};
}

std::string Pkce::Base64UrlEncode(const std::string& input) {
    std::string encoded;
    CryptoPP::StringSource ss(
        reinterpret_cast<const CryptoPP::byte*>(input.data()),
        input.size(),
        true,
        new CryptoPP::Base64URLEncoder(new CryptoPP::StringSink(encoded)));
    return encoded;
}
