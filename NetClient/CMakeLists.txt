add_executable(client Client.cpp)
target_link_libraries(client PRIVATE asio::asio)
target_sources(client PRIVATE 
../NetCommon/common.h
../NetCommon/message.h
../NetCommon/includes.h
../NetCommon/threadsafequeue.h
../NetCommon/connection.h
../NetCommon/client.h
../NetCommon/server.h
)
target_include_directories(client PRIVATE ../NetCommon)