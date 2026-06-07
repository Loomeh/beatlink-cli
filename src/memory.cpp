#include "memory.hpp"
#include "debug.hpp"
#include <string>
#include <cstring>
#include <sys/uio.h>

void Memory::ApplyPatches(WriteMemory& writer, pid_t pid, uint64_t baseAddress, std::string newAuthCode) {
    struct iovec memLocal, memRemote;

    if (gDebugEnabled) {
        std::cout << "[debug] ApplyPatches pid=" << pid
                  << " base=0x" << std::hex << baseAddress << std::dec << "\n";
    }

    std::string redirector_url = "redirector.ploxxy.dev";

    unsigned char redirector_url_bytes[redirector_url.size() + 1];
    strcpy((char*)redirector_url_bytes, redirector_url.c_str());

    // Update redirector hostname
    if (gDebugEnabled) {
        std::cout << "[debug] Writing redirector URL '" << redirector_url
                  << "' (" << sizeof(redirector_url_bytes) << " bytes) to 0x"
                  << std::hex << (baseAddress + 0x1D80890) << std::dec << "\n";
    }
    writer.writeMem(redirector_url_bytes, sizeof(redirector_url_bytes), pid, baseAddress + 0x1D80890, memLocal, memRemote);

    // Disable SSL in ProtoSSL::Connect function
    unsigned char patch_ssl[] = { 0x31 };
    if (gDebugEnabled) {
        std::cout << "[debug] Writing patch_ssl (" << sizeof(patch_ssl)
                  << " bytes) to 0x" << std::hex << (baseAddress + 0x2DBE9B0)
                  << std::dec << "\n";
    }
    writer.writeMem(patch_ssl, sizeof(patch_ssl), pid, baseAddress + 0x2DBE9B0, memLocal, memRemote);

    // Disable LSX authentication code verification
    unsigned char patch_lsx[] = { 0x48, 0x90 };
    if (gDebugEnabled) {
        std::cout << "[debug] Writing patch_lsx (" << sizeof(patch_lsx)
                  << " bytes) to 0x" << std::hex << (baseAddress + 0x388BCE6)
                  << std::dec << "\n";
    }
    writer.writeMem(patch_lsx, sizeof(patch_lsx), pid, baseAddress + 0x388BCE6, memLocal, memRemote);

    // Bypass encryption of authenticated requests -> (no longer needed)
    //unsigned char patch_enc[] = { 0xE9, 0xC7, 0x00 };
    //if (gDebugEnabled) {
    //    std::cout << "[debug] Writing patch_enc (" << sizeof(patch_enc)
    //              << " bytes) to 0x" << std::hex << (baseAddress + 0x39C0D81)
    //              << std::dec << "\n";
    //}
    //writer.writeMem(patch_enc, sizeof(patch_enc), pid, baseAddress + 0x39C0D81, memLocal, memRemote);

    // Overwrite memory address of authentication code
    unsigned char patch_auth[] = { 0x15, 0x28, 0x04, 0x3B, 0xFE };
    if (gDebugEnabled) {
        std::cout << "[debug] Writing patch_auth (" << sizeof(patch_auth)
                  << " bytes) to 0x" << std::hex << (baseAddress + 0x388AED3)
                  << std::dec << "\n";
    }
    writer.writeMem(patch_auth, sizeof(patch_auth), pid, baseAddress + 0x388AED3, memLocal, memRemote);

    // Write pointer to new data
    unsigned char patch_ptr[] = { 0x40, 0xB3, 0xC3, 0x41, 0x01 };
    if (gDebugEnabled) {
        std::cout << "[debug] Writing patch_ptr (" << sizeof(patch_ptr)
                  << " bytes) to 0x" << std::hex << (baseAddress + 0x1C3B300)
                  << std::dec << "\n";
    }
    writer.writeMem(patch_ptr, sizeof(patch_ptr), pid, baseAddress + 0x1C3B300, memLocal, memRemote);

    // Write new authentication code
    unsigned char new_auth_code_bytes[newAuthCode.size() + 1];
    strcpy((char*)new_auth_code_bytes, newAuthCode.c_str());
    if (gDebugEnabled) {
        std::cout << "[debug] Writing newAuthCode '" << newAuthCode
                  << "' (" << sizeof(new_auth_code_bytes) << " bytes) to 0x"
                  << std::hex << (baseAddress + 0x1C3B340) << std::dec << "\n";
    }
    writer.writeMem(new_auth_code_bytes, sizeof(new_auth_code_bytes), pid, baseAddress + 0x1C3B340, memLocal, memRemote);

    if (gDebugEnabled) {
        std::cout << "[debug] ApplyPatches completed successfully\n";
    }
}
