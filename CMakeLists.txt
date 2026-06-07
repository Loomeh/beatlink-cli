cmake_minimum_required(VERSION 3.15)
project(beatlink-cli CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Download and Build OpenSSL Statically from Source
# ---------------------------------------------------------------------------
ExternalProject_Add(openssl_external
    URL https://github.com/openssl/openssl/releases/download/openssl-4.0.0/openssl-4.0.0.tar.gz
    CONFIGURE_COMMAND <SOURCE_DIR>/config no-shared no-tests --prefix=<INSTALL_DIR>
    BUILD_COMMAND make
    INSTALL_COMMAND make install_sw
)
ExternalProject_Get_Property(openssl_external INSTALL_DIR)
set(OPENSSL_INCLUDE_DIR "${INSTALL_DIR}/include")
set(LIB_SSL "${INSTALL_DIR}/lib64/libssl.a")
set(LIB_CRYPTO "${INSTALL_DIR}/lib64/libcrypto.a")

# ---------------------------------------------------------------------------
# Download and Build Crypto++ Statically from Source
# ---------------------------------------------------------------------------
ExternalProject_Add(cryptopp_external
    GIT_REPOSITORY https://github.com/weidai11/cryptopp.git
    GIT_TAG CRYPTOPP_8_9_0
    
    # Force CMake to run 'make' inside the downloaded source code folder
    BUILD_IN_SOURCE 1 
    
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make libcryptopp.a
    
    # Copy from SOURCE_DIR now, since that is where it was built
    INSTALL_COMMAND cmake -E copy <SOURCE_DIR>/libcryptopp.a <INSTALL_DIR>/lib/libcryptopp.a
            COMMAND cmake -E copy_directory <SOURCE_DIR> <INSTALL_DIR>/include/cryptopp
)
ExternalProject_Get_Property(cryptopp_external INSTALL_DIR)
set(CRYPTOPP_INCLUDE_DIR "${INSTALL_DIR}/include")
set(LIB_CRYPTOPP "${INSTALL_DIR}/lib/libcryptopp.a")

# ---------------------------------------------------------------------------
# Main Executable Target
# ---------------------------------------------------------------------------
file(GLOB SOURCES "src/*.cpp")
add_executable(beatlink-cli ${SOURCES})

# Ensure the external libraries are compiled BEFORE beatlink-cli tries to link them
add_dependencies(beatlink-cli openssl_external cryptopp_external)

target_include_directories(beatlink-cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
    ${OPENSSL_INCLUDE_DIR}
    ${CRYPTOPP_INCLUDE_DIR}
)

target_compile_options(beatlink-cli PRIVATE -march=x86-64 -mtune=generic -O2)

# Manually link the generated .a files safely alongside dynamic system libraries
target_link_libraries(beatlink-cli PRIVATE
    ${LIB_SSL}
    ${LIB_CRYPTO}
    ${LIB_CRYPTOPP}
    Threads::Threads
    dl # OpenSSL needs the dynamic linker library
)