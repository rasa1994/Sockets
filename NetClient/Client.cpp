#include <iostream>
#include "includes.h"
#include "client.h"
#include "server.h"

enum class MessageType : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

class CustomServer : public net::ServerInterface<MessageType>
{
public:
    CustomServer(uint16_t port) : net::ServerInterface<MessageType>(port)
    {

    }
protected:
    virtual bool OnClientConnect(std::shared_ptr<net::connection<MessageType>> client)
    {
        return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<net::connection<MessageType>> client)
    {

    }

    virtual void OnMessage(std::shared_ptr<net::connection<MessageType>> client, const net::message<MessageType>& message)
    {

    }
};

int main()
{
    CustomServer server(60000);
    server.Start();

    while (true)
    {
        server.Update();
    }

    return 0;
}

