cmake_minimum_required(VERSION 3.31.3)

project(async_ws_chat LANGUAGES CXX)

find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(async-ws-chat
	src/main.cpp
)

target_compile_definitions(async-ws-chat PRIVATE DEBUG)

target_include_directories(async-ws-chat PRIVATE src)

target_include_directories(async-ws-chat PRIVATE include)

target_link_libraries(async-ws-chat PRIVATE OpenSSL::Crypto OpenSSL::SSL)
