#pragma once
#include "common.h"
#include "message.h"
#include "ThreadSafeQueue.h"
#include "connection.h"

namespace net
{
	template <typename Data>
	class ClientInterface
	{
	public:
		ClientInterface() : m_socket(m_asioContext)
		{

		}

		virtual ~ClientInterface()
		{
			Disconnect();
		}

		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				asio::ip::tcp::resolver resolver(m_asioContext);
				auto endPoints = resolver.resolve(host, std::to_string(port));

				m_connection = std::make_unique<connection<Data>>(
					connection<Data>::Owner::Client,
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
			return false;
		}

		void Disconnect()
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

		bool IsConnected() const
		{
			if (m_connection)
			{
				return m_connection->IsConnected();
			}
			return false;
		}

		ThreadSafeQueue<owned_message<Data>>& Incoming()
		{
			return m_messagesIn;
		}

	protected:
		asio::io_context m_asioContext;
		std::jthread m_threadContext;
		asio::ip::tcp::socket m_socket;
		std::unique_ptr<connection<Data>> m_connection;
	private:
		ThreadSafeQueue<owned_message<Data>> m_messagesIn;
	};
}