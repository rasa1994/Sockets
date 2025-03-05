#pragma once 

#include "common.h"
#include "ThreadSafeQueue.h"
#include "message.h"

namespace net
{
    template <typename Data>
    class connection : public std::enable_shared_from_this<connection<Data>>
    {
    public:
        connection()
        {
        }

        virtual ~connection()
        {

        }

        bool ConnectToServer();
        bool Disconnect();
        bool IsConnected() const;

        bool Send(const message<Data>& msg);

    protected:
        asio::ip::tcp::socket m_socket;

        asio::io_context& m_asioContext;

        ThreadSafeQueue<message<Data>> m_messagesOut;

        ThreadSafeQueue<owned_message<Data>>& m_messagesIn;
    };
}