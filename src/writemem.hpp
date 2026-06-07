#ifndef WRITEMEM_HPP
#define WRITEMEM_HPP

#include <condition_variable>
#include <sys/types.h>
#include <sys/uio.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <cerrno>
#include <cstring>

using std::cout;
using std::endl;

struct StockPid {
  pid_t pid;
  char buff[512];
  FILE * pid_pipe;
};

class ProcessManager {
  public: void Func_StockPid(const char * processtarget);
  std::mutex mtx;
  std::condition_variable cv;
  bool process_found = false;
  int countdown = 5;
  StockPid stockthepid {};
};

inline void ProcessManager::Func_StockPid(const char * processtarget) {
  while (!process_found) {
    stockthepid.pid_pipe = popen(processtarget, "r");
    if (!fgets(stockthepid.buff, 512, stockthepid.pid_pipe)) {
      for (int i = countdown; i >= 0; i--) {
        cout << "Mirror's Edge Catalyst is not open. Retrying in " << i <<
          " second(s)...\r";
        std::flush(cout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      cout << "                                           \r";
      countdown = 5;
      pclose(stockthepid.pid_pipe);
      continue;
    }

    stockthepid.pid = strtoul(stockthepid.buff, nullptr, 10);

    if (stockthepid.pid == 0) {
      for (int i = countdown; i >= 0; i--) {
        cout << "Mirror's Edge Catalyst is not open. Retrying in " << i <<
          " second(s)...\r";
        std::flush(cout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      cout << "                                           \r";
      countdown = 5;
      system("clear");
    } else {
      std::unique_lock < std::mutex > lk(mtx);
      cout << "Mirror's Edge Catalyst is running - PID NUMBER -> " <<
        stockthepid.pid << endl;

      for (int i = 5; i > 0; i--) {
        cout << "Waiting " << i << " second(s) before injecting...\r";
        std::flush(cout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }

      // Clear the line and proceed
      cout << "Injecting now...                                       \n";

      process_found = true;
      cv.notify_one();
      pclose(stockthepid.pid_pipe);
      break;
    }
    pclose(stockthepid.pid_pipe);
  }
}

class WriteMemory {
  public: void writeMem(const unsigned char * data, size_t size, pid_t pid, uint64_t address, struct iovec & memLocal, struct iovec & memRemote) {
    memLocal.iov_base = (void * ) data;
    memLocal.iov_len = size;

    memRemote.iov_base = (void * )(uintptr_t) address;
    memRemote.iov_len = size;

    ssize_t bytesWritten = process_vm_writev(pid, & memLocal, 1, & memRemote, 1, 0);

    if (bytesWritten != size) {
      std::cerr << "Error writing memory at address 0x" << std::hex << address << std::dec << "\n";
      std::cerr << "Reason: " << strerror(errno) << " (Errno: " << errno << ")\n";
      std::cerr << "Bytes expected: " << size << ", Bytes written: " << bytesWritten << "\n";
      exit(1);
    }
  }

  uintptr_t getBaseAddress(pid_t pid) {
    std::ifstream mapsFile;
    std::string mapsPath = "/proc/" + std::to_string(pid) + "/maps";
    mapsFile.open(mapsPath);

    if (!mapsFile.is_open()) {
      std::cerr << "Could not open " << mapsPath << std::endl;
      return 0;
    }

    std::string line;
    uintptr_t baseAddress = 0;

    // Loop through the entire maps file
    while (std::getline(mapsFile, line)) {
      // Look for the region associated with the game's executable
      if (line.find("MirrorsEdgeCatalyst.exe") != std::string::npos) {
        std::istringstream iss(line);
        std::string addressRange;
        iss >> addressRange;

        // Split the address range
        size_t dashPos = addressRange.find('-');
        if (dashPos != std::string::npos) {
          std::string baseAddressStr = addressRange.substr(0, dashPos);
          baseAddress = std::stoull(baseAddressStr, nullptr, 16);
          break;
        }
      }
    }

    mapsFile.close();
    return baseAddress;
  }
};

#endif // WRITEMEM_HPP
