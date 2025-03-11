#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include "NetCommon\threadsafequeue.h"
#include "NetCommon\connection.h"
#include "NetCommon\client.h"
#include "NetCommon\server.h"
#include "NetCommon\message.h"

int main()
{
	net::ServerInterface<net::message_header<size_t>> server(60000);
	server.Start();

	net::ClientInterface<net::message_header<size_t>> client;
}