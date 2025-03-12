#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include "Includes\ThreadSafeQueue.hpp"
#include "Includes\Connection.hpp"
#include "Includes\ClientInterface.hpp"
#include "Includes\ServerInterface.hpp"
#include "Includes\Message.hpp"

int main()
{
	sockets::ServerInterface<sockets::message_header<size_t>> server(60000);
	server.Start();

	sockets::ClientInterface<sockets::message_header<size_t>> client;
}