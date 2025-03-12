cmake_minimum_required(VERSION 3.29)

enable_testing()

set(VCPKG_ROOT "VCPKG_ROOT" CACHE STRING "VCPKG_ROOT")

if (NOT DEFINED VCPKG_ROOT)
  set(VCPKG_ROOT "C:/vcpkg")
endif()

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project("Sockets" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(asio CONFIG REQUIRED)
add_executable(main Main.cpp)

add_subdirectory(Includes)
add_subdirectory(Tests)

target_sources(main PRIVATE
 Includes/ClientInterface.hpp
 Includes/CommonIncludes.h
 Includes/Connection.hpp
 Includes/Message.hpp
 Includes/ServerInterface.hpp
 Includes/ThreadSafeQueue.hpp
 )
target_link_libraries(main PRIVATE asio::asio)
