cmake_minimum_required(VERSION 3.31.3)

project(async_ws_chat VERSION 1.0 LANGUAGES CXX)

add_executable(async-ws-chat
	src/main.cpp
)

target_include_directories(async-ws-chat PRIVATE src)
