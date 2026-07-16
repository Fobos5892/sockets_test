cmake_minimum_required(VERSION 3.16)
project(modbus_chat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CONFIGURATION_TYPES)
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${config} config_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_SOURCE_DIR}/out/${config})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_SOURCE_DIR}/out/${config})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_SOURCE_DIR}/out/${config})
    endforeach()
else()
    set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
endif()

add_library(common STATIC
    common/Config.cpp
    common/ModbusTcp.cpp
    common/Protocol.cpp
    common/transport/ModbusMessageTransport.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

add_library(server_lib STATIC
    server/Server.cpp
    server/model/ClientRegistry.cpp
    server/transport/TcpListenSocket.cpp
    server/view/ServerLogView.cpp
    server/viewmodel/MessageDispatcher.cpp
    server/viewmodel/MessageDispatcherFactory.cpp
    server/viewmodel/PresenceBroadcaster.cpp
    server/viewmodel/ServerViewModel.cpp
    server/viewmodel/handlers/RegisterMessageHandler.cpp
    server/viewmodel/handlers/ChatMessageHandler.cpp
    server/viewmodel/handlers/ListUsersMessageHandler.cpp
)

target_include_directories(server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)

target_link_libraries(server_lib PUBLIC common)

add_executable(server
    server/main.cpp
)

target_link_libraries(server PRIVATE server_lib)

add_library(client_lib STATIC
    client/model/ClientSession.cpp
    client/transport/TcpClientConnector.cpp
    client/view/ConsoleClientView.cpp
    client/viewmodel/ClientViewModel.cpp
    client/viewmodel/IncomingMessagePresenter.cpp
    client/viewmodel/OutgoingMessageBuilder.cpp
)

target_include_directories(client_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/client
)

target_link_libraries(client_lib PUBLIC common)

add_executable(client
    client/main.cpp
)

target_link_libraries(client PRIVATE client_lib)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(modbus_tests
    tests/test_config.cpp
    tests/test_modbus.cpp
    tests/test_protocol.cpp
    tests/test_message.cpp
    tests/test_integration.cpp
)

target_include_directories(modbus_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(modbus_tests PRIVATE common server_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(modbus_tests)
