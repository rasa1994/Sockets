#include <iostream>
#include "olc.h"
#include "client.h"

enum class MessageType : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

int main()
{
    net::message<MessageType> msg;
    msg.header.id = MessageType::ServerAccept;

    int a = 1;
    bool b = true;
    float c = 3.141592f;

    struct 
    {
        int x;
        int y;
    } d[5];

    msg << a << b << c << d;

    a = 10;
    b = false;
    c = 1.0f;
	msg >> d >> c >> b >> a;

    std::cout << std::boolalpha;
	std::cout << a << " " << b << " " << c << std::endl;
}

