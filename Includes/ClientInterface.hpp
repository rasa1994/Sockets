#pragma once
#include "CommonIncludes.h"
#include "Message.hpp"
#include "ThreadSafeQueue.hpp"
#include "Connection.hpp"

namespace sockets
{
	template <typename Data>
	class ClientInterface
	{
	public:
		ClientInterface() = default;

		virtual ~ClientInterface();

		ClientInterface(ClientInterface&) = delete;
		ClientInterface& operator=(ClientInterface&) = delete;
		ClientInterface(ClientInterface&&) = delete;
		ClientInterface& operator=(ClientInterface&&) = delete;

		bool Connect(const std::string& host, uint16_t port);

		void Send(const message<Data>& msg);

		void Disconnect();

		bool IsConnected() const;

		ThreadSafeQueue<owned_message<Data>>& Incoming();

	protected:
		asio::io_context m_asioContext;
		std::jthread m_threadContext;
		std::unique_ptr<Connection<Data>> m_connection;
	private:
		ThreadSafeQueue<owned_message<Data>> m_messagesIn;
	};

	
	template <typename Data>
	ClientInterface<Data>::~ClientInterface()
	{
		Disconnect();
	}

	template <typename Data>
	bool ClientInterface<Data>::Connect(const std::string& host, const uint16_t port)
	{
		try
		{
			asio::ip::tcp::resolver resolver(m_asioContext);
			auto endPoints = resolver.resolve(host, std::to_string(port));

			m_connection = std::make_unique<Connection<Data>>(
				Connection<Data>::Owner::Client,
				m_asioContext,
				asio::ip::tcp::socket(m_asioContext),
				m_messagesIn
			);

			m_connection->ConnectToServer(endPoints);

			m_threadContext = std::jthread([this]()
			{
				m_asioContext.run();
			});
		}
		catch (std::exception& e)
		{
			std::cerr << "Client Exception: " << e.what() << "\n";
			return false;
		}
		return true;
	}

	template <typename Data>
	void ClientInterface<Data>::Send(const message<Data>& msg)
	{
		if (IsConnected())
		{
			m_connection->Send(msg);
		}
	}

	template <typename Data>
	void ClientInterface<Data>::Disconnect()
	{
		if (IsConnected())
		{
			m_connection->Disconnect();
		}

		m_asioContext.stop();

		if (m_threadContext.joinable())
		{
			m_threadContext.join();
		}

		m_connection.release();
	}

	template <typename Data>
	bool ClientInterface<Data>::IsConnected() const
	{
		if (m_connection)
		{
			return m_connection->IsConnected();
		}
		return false;
	}

	template <typename Data>
	ThreadSafeQueue<owned_message<Data>>& ClientInterface<Data>::Incoming()
	{
		return m_messagesIn;
	}
}
