cmake_minimum_required(VERSION 3.16)
project(thread_per_connection_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Если тип сборки не указан, по умолчанию Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

# Вся логика сервера - в библиотеке, чтобы можно было подключать к серверу и тестам отдельно
add_library(server_core STATIC
    src/http.cpp
    src/router.cpp
    src/server.cpp
)
target_include_directories(server_core PUBLIC src)
target_compile_options(server_core PUBLIC -Wall -Wextra)
target_link_libraries(server_core PUBLIC Threads::Threads)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE server_core)