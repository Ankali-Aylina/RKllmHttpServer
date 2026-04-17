cmake_minimum_required(VERSION 3.11)
project(RKLLM_HTTP_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 检查并加载 Crow 子模块（必须通过 git submodule update --init 初始化）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Crow/CMakeLists.txt)
    message(STATUS "Found Crow submodule, adding to build")
    add_subdirectory(Crow)
else()
    message(FATAL_ERROR "Crow submodule not found. Please run: git submodule update --init")
endif()

# 检查并加载 nlohmann/json 子模块（必须通过 git submodule update --init 初始化）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/json/CMakeLists.txt)
    message(STATUS "Found nlohmann/json submodule, adding to build")
    add_subdirectory(json)
else()
    message(FATAL_ERROR "nlohmann/json submodule not found. Please run: git submodule update --init")
endif()

# RKLLM 路径 - 使用项目内的include和lib文件夹
set(RKLLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(RKLLM_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(${RKLLM_INCLUDE_DIR})
link_directories(${RKLLM_LIB_DIR})

# 包含 Crow 头文件目录（子模块）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Crow/include)

# 包含 nlohmann/json 头文件目录（子模块）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/json/include)

# 查找 Boost（Crow 需要）
find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

# 可执行文件
add_executable(rkllm_http_server
    main.cpp
    llm_model.cpp
    http_server.cpp
)

# 链接库
target_link_libraries(rkllm_http_server
    ${Boost_LIBRARIES}
    rkllmrt
    pthread
    dl
)

# 可选：使用独立 ASIO 而不是 Boost.Asio（需要系统安装 libasio-dev）
# target_compile_definitions(rkllm_http_server PRIVATE CROW_USE_ASIO)