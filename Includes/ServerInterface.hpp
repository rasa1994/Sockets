#pragma once

#include "CommonIncludes.h"
#include "ThreadSafeQueue.hpp"
#include "Message.hpp"
#include "Connection.hpp"

namespace sockets
{
	template <typename Data>
	class ServerInterface
	{
	public:
		ServerInterface(uint16_t port);

		virtual ~ServerInterface();

		ServerInterface(ServerInterface&) = delete;
		ServerInterface& operator=(ServerInterface&) = delete;
		ServerInterface(ServerInterface&&) = delete;
		ServerInterface& operator=(ServerInterface&&) = delete;

		bool Start();

		void Stop();

		void WaitForClientConnection();

		void MessageClient(std::shared_ptr<Connection<Data>> client, const message<Data>& msg);

		void MessageAllClients(const message<Data>& msg, std::shared_ptr<Connection<Data>> clientToIgnore = nullptr);

		void Update(size_t maxMessages = std::numeric_limits<size_t>::max(), bool wait = false);

		virtual bool OnClientConnect(std::shared_ptr<Connection<Data>> client) = 0;
		virtual void OnClientDisconnect(std::shared_ptr<Connection<Data>> client) = 0;
		virtual void OnMessage(std::shared_ptr<Connection<Data>> client, message<Data>& data) = 0;
		virtual void OnClientValidated(std::shared_ptr<Connection<Data>> client) = 0;

	protected:
		ThreadSafeQueue<sockets::owned_message<Data>> m_messagesIn;

		std::deque<std::shared_ptr<Connection<Data>>> m_connections;

		asio::io_context m_asioContext;
		std::jthread m_threadContext;

		asio::ip::tcp::acceptor m_asioAcceptor;

		uint32_t IdCounter{ 10000 };
	};

	template <typename Data>
	ServerInterface<Data>::ServerInterface(uint16_t port): 
		m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{

	}

	template <typename Data>
	ServerInterface<Data>::~ServerInterface()
	{
		Stop();
	}

	template <typename Data>
	bool ServerInterface<Data>::Start()
	{
		try
		{
			WaitForClientConnection();

			m_threadContext = std::jthread([this]() {m_asioContext.run(); });
		}
		catch (const std::exception& e)
		{
			std::cerr << "[SERVER} Exception: " << e.what() << "\n";
			return false;
		}


		std::cout << "[SERVER] Started \n" << std::endl;
		return true;
	}

	template <typename Data>
	void ServerInterface<Data>::Stop()
	{
		m_asioContext.stop();

		if (m_threadContext.joinable())
			m_threadContext.join();

		std::cout << "[SERVER] Stopped \n" << std::endl;
	}

	template <typename Data>
	void ServerInterface<Data>::WaitForClientConnection()
	{
		m_asioAcceptor.async_accept(
			[this](std::error_code errorCode, asio::ip::tcp::socket socket)
			{
				if (!errorCode)
				{
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;

					std::shared_ptr<Connection<Data>> newConnection = std::make_shared<Connection<Data>>(Connection<Data>::Owner::Server, m_asioContext, std::move(socket), m_messagesIn);

					if (OnClientConnect(newConnection))
					{
						m_connections.push_back(std::move(newConnection));

						m_connections.back()->ConnectToClient(this, IdCounter++);

						std::cout << "[" << m_connections.back()->GetId() << "] Connection Approved!" << std::endl;
					}
					else
					{
						std::cout << "[-----] Connection Denied!" << std::endl;
					}
				}
				else
				{
					std::cout << "[SERVER] New Connection error" << errorCode.message() << std::endl;
				}

				WaitForClientConnection();
			});
	}

	template <typename Data>
	void ServerInterface<Data>::MessageClient(std::shared_ptr<Connection<Data>> client, const message<Data>& msg)
	{
		if (client && client->IsConnected())
		{
			client->Send(msg);
		}
		else
		{
			OnClientDisconnect(client);
			client.reset();
			m_connections.erase(std::ranges::remove(m_connections, client), m_connections.end());
		}
	}

	template <typename Data>
	void ServerInterface<Data>::MessageAllClients(const message<Data>& msg,
		std::shared_ptr<Connection<Data>> clientToIgnore)
	{
		bool invalidClient = false;

		for (auto& client : m_connections)
		{
			if (client && client->IsConnected())
			{
				if (client != clientToIgnore)
					client->Send(msg);
			}
			else
			{
				OnClientDisconnect(client);
				client.reset();
				invalidClient = true;
			}
		}

		if (invalidClient)
			m_connections.erase(std::remove(m_connections.begin(), m_connections.end(), nullptr), m_connections.end());
	}

	template <typename Data>
	void ServerInterface<Data>::Update(size_t maxMessages, bool wait)
	{

		if (wait)
			m_messagesIn.wait();

		size_t messageCount = 0;
		while (messageCount < maxMessages && !m_messagesIn.empty())
		{
			auto message = m_messagesIn.pop_front();

			OnMessage(message.remote, message.msg);

			messageCount++;
		}
	}
}
