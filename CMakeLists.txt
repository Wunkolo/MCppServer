cmake_minimum_required(VERSION 3.14)
project(MCppServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(MCppServer src/main.cpp
        src/core/server.cpp
        src/core/server.h
        src/networking/client.cpp
        src/networking/client.h
        src/networking/network.cpp
        src/networking/network.h
        src/core/utils.cpp
        src/core/utils.h
        src/core/config.cpp
        src/core/config.h
        src/registries/biome.cpp
        src/registries/biome.h
        src/registries/dimension_type.cpp
        src/registries/dimension_type.h
        src/registries/painting_variant.cpp
        src/registries/painting_variant.h
        src/registries/wolf_variant.cpp
        src/registries/wolf_variant.h
        src/registries/damage_type.cpp
        src/registries/damage_type.h
        src/registries/registry_manager.cpp
        src/registries/registry_manager.h
        src/entities/player.cpp
        src/entities/player.h
        src/networking/fetch.cpp
        src/networking/fetch.h
        src/world/flatworld.cpp
        src/world/flatworld.h
        src/world/chunk.cpp
        src/world/chunk.h
        src/data/data.cpp
        src/data/data.h
        src/entities/entity.cpp
        src/entities/entity.h
        src/entities/entity_manager.cpp
        src/entities/entity_manager.h
        src/enums/enums.h
        src/world/world.cpp
        src/world/world.h
        src/world/region_file.cpp
        src/world/region_file.h
        src/entities/slot_data.cpp
        src/entities/slot_data.h
        src/entities/equipment.cpp
        src/entities/equipment.h
        src/entities/inventory.cpp
        src/entities/inventory.h
        src/world/block_states.cpp
        src/world/block_states.h
        src/encryption/rsa_key.cpp
        src/encryption/rsa_key.h
        thirdparty/daft_hash.h
        thirdparty/daft_hash.cpp
        src/commands/CommandBuilder.cpp
        src/commands/CommandBuilder.h
        src/utils/thread_pool.cpp
        src/utils/thread_pool.h
        src/server/rcon_server.cpp
        src/server/rcon_server.h
        src/utils/le32toh.h
        src/server/query_server.cpp
        src/server/query_server.h
        src/networking/packet_ids.h
        src/networking/clientbound_packets.cpp
        src/networking/clientbound_packets.h
        src/world/world_border.cpp
        src/world/world_border.h
        src/world/world_time.cpp
        src/world/world_time.h
        src/utils/translation.cpp
        src/utils/translation.h
        src/world/boss_bar.cpp
        src/world/boss_bar.h
)

# Include FetchContent module
include(FetchContent)

# Fetch nlohmann/json library
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3  # Use the desired version tag
)
FetchContent_MakeAvailable(json)

# Fetch ZLIB library
FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1  # Adjust to the latest version as needed
)
FetchContent_MakeAvailable(zlib)

# Manually set ZLIB include directory
set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})

set(ZLIB_LIBRARY ${ZLIB_LIBRARY})

# Fetch libnbt++ 2 library
FetchContent_Declare(
        libnbtplusplus
        GIT_REPOSITORY https://github.com/PrismLauncher/libnbtplusplus.git
        GIT_TAG master
)
set(NBT_BUILD_SHARED OFF CACHE BOOL "Build libnbt++ as a shared library" FORCE)
set(NBT_USE_ZLIB ON CACHE BOOL "Enable ZLIB support in libnbt++" FORCE)
set(NBT_BUILD_TESTS OFF CACHE BOOL "Disable libnbt++ tests" FORCE)
FetchContent_MakeAvailable(libnbtplusplus)

FetchContent_Declare(
        openssl-cmake
        GIT_REPOSITORY https://github.com/viaduck/openssl-cmake.git
        GIT_TAG        v3
)
FetchContent_MakeAvailable(openssl-cmake)

FetchContent_Declare(
        cppcodec
        GIT_REPOSITORY https://github.com/tplgy/cppcodec.git
        GIT_TAG        v0.2
)

set(BUILD_TESTING OFF CACHE BOOL "Disable cppcodec tests" FORCE)

# Make cppcodec available
FetchContent_MakeAvailable(cppcodec)

add_dependencies(MCppServer zlib nlohmann_json cppcodec openssl)

# Add the include directories for the dependencies
target_include_directories(MCppServer PRIVATE src ${libnbtplusplus_SOURCE_DIR}/include ${libnbtplusplus_BINARY_DIR} ${ssl_SOURCE_DIR}/include ${cppcodec_SOURCE_DIR} thirdparty)

if(WIN32)
    set(OPENSSL_DLL_DIR "${CMAKE_BINARY_DIR}/_deps/openssl-cmake-build/openssl-prefix/src/openssl/usr/local/bin")
    target_link_libraries(MCppServer PRIVATE ws2_32 nlohmann_json::nlohmann_json nbt++ zlibstatic  ssl crypto crypt32)
    # POST_BUILD command to copy OpenSSL DLLs after they are built
    add_custom_command(TARGET MCppServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DLL_DIR}/libcrypto-3-x64.dll"
            "${CMAKE_BINARY_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DLL_DIR}/libssl-3-x64.dll"
            "${CMAKE_BINARY_DIR}/"
    )
else ()
    target_link_libraries(MCppServer PRIVATE nlohmann_json::nlohmann_json nbt++ zlibstatic ssl crypto)
endif()
