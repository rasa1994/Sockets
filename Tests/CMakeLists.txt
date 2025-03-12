
add_executable(tests Main.cpp)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

target_sources(tests PRIVATE
 ../Includes/ClientInterface.hpp
 ../Includes/CommonIncludes.h
 ../Includes/Connection.hpp
 ../Includes/Message.hpp
 ../Includes/ServerInterface.hpp
 ../Includes/ThreadSafeQueue.hpp
 )

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/Includes)
target_link_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/Includes)

include(GoogleTest)
gtest_discover_tests(tests)