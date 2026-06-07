#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "TokenResponse.hpp"
#include "debug.hpp"
#include "writemem.hpp"
#include "memory.hpp"
#include "authentication.hpp"

static pid_t GetRunningPid(const char *processName) {
    FILE *pidPipe = popen(processName, "r");
    if (!pidPipe) {
        std::cerr << "Failed to check for running process: " << strerror(errno) << "\n";
        return 0;
    }

    char buff[512];
    if (!fgets(buff, sizeof(buff), pidPipe)) {
        pclose(pidPipe);
        return 0;
    }

    pclose(pidPipe);
    return strtoul(buff, nullptr, 10);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-debug") {
            gDebugEnabled = true;
        }
    }

    if (gDebugEnabled) {
        std::cerr << "Warning: -debug is enabled. Do not post your auth code or tokens anywhere.\n";
    }

    ProcessManager pm;
    WriteMemory writer;
    Memory memoryManager;
    Authentication auth;

    TokenResponse token = auth.ReadTokenFile();
    if (token.access_token.empty() || token.expires_at <= std::chrono::system_clock::now()) {
        if (gDebugEnabled) {
            std::cout << "[debug] Token missing or expired; requesting a new token\n";
        }
        auth.GetToken();
        token = auth.ReadTokenFile();
    }

    const char *processName = "ps -C MirrorsEdgeCatalyst.exe -o pid --no-headers";

    pid_t existingPid = GetRunningPid(processName);
    std::thread monitor_thread;

    if (existingPid > 0) {
        pm.stockthepid.pid = existingPid;
        pm.process_found = true;
        std::cout << "Mirror's Edge Catalyst is already running - PID NUMBER -> "
                  << existingPid << std::endl;
    } else {
        system("xdg-open 'steam://run/1233570'");
        monitor_thread = std::thread(&ProcessManager::Func_StockPid, &pm, processName);
        std::unique_lock<std::mutex> lk(pm.mtx);
        if (!pm.cv.wait_for(lk, std::chrono::minutes(3), [&pm] { return pm.process_found; })) {
            if (gDebugEnabled) {
                std::cout << "[debug] Mirror's Edge Catalyst was not detected within 3 minutes\n";
            }

            pm.process_found = true;
            lk.unlock();
            pm.cv.notify_one();

            if (monitor_thread.joinable()) {
                monitor_thread.join();
            }

            std::cerr << "Mirror's Edge Catalyst was not detected within 3 minutes. Exiting.\n";
            return 1;
        }
    }

    uint64_t baseAddress = writer.getBaseAddress(pm.stockthepid.pid);

    if (gDebugEnabled) {
        std::cout << "[debug] token expires_at <= now: "
                  << (token.expires_at <= std::chrono::system_clock::now() ? "true" : "false")
                  << "\n";
        std::cout << "[debug] baseAddress=0x" << std::hex << baseAddress << std::dec << "\n";
    }

    if (token.expires_at > std::chrono::system_clock::now()) {
        if (gDebugEnabled) {
            std::cout << "[debug] Token valid; applying memory patches\n";
        }
        memoryManager.ApplyPatches(writer, pm.stockthepid.pid, baseAddress, token.access_token);
    }
    else {
        if (gDebugEnabled) {
            std::cout << "[debug] Token expired or missing; skipping memory patches\n";
        }
        
        std::cerr << "Cannot apply patches because the access token is expired or missing.\n";
        return 1;
    }

    std::cout << "All memory patches applied successfully." << std::endl;

    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    return 0;
}
