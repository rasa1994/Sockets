#pragma once 

#include "CommonIncludes.h"
#include "ThreadSafeQueue.hpp"
#include "Message.hpp"

namespace sockets
{
	template <typename Data>
	class ServerInterface;

	template <typename Data>
	class Connection : public std::enable_shared_from_this<Connection<Data>>
	{
	public:
		enum class Owner : uint8_t
		{
			Server,
			Client
		};


		Connection(Owner owner, asio::io_context& asioContext, asio::ip::tcp::socket socket, ThreadSafeQueue<sockets::owned_message<Data>>& messageQueue);

		virtual ~Connection() = default;

		Connection(Connection&) = delete;
		Connection& operator=(Connection&) = delete;
		Connection(Connection&&) = delete;
		Connection& operator=(Connection&&) = delete;

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endPoints);

		void Disconnect();

		bool IsConnected() const;

		void Send(const message<Data>& msg);
		uint32_t GetId() const;

		void ConnectToClient(sockets::ServerInterface<Data>* server, uint32_t id = 0);

		static uint64_t Encrypt(uint64_t data);
		static uint64_t Decrypt(uint64_t data);

		void WriteValidation();

		void ReadValidation(sockets::ServerInterface<Data>* server = nullptr);

	protected:
		uint32_t m_id{ 0 };
		Owner m_owner = Owner::Server;
		asio::ip::tcp::socket m_socket;
		asio::io_context& m_asioContext;
		ThreadSafeQueue<message<Data>> m_messagesOut;
		ThreadSafeQueue<owned_message<Data>>& m_messagesIn;

		uint64_t m_handShakeOut{ 0 };
		uint64_t m_handShakeIn{ 0 };
		uint64_t m_handShakeCheck{ 0 };
		std::atomic_bool m_testPassed{ false };

	private:
		void ReadHeader();

		void ReadBody();

		void WriteHeader();

		void WriteBody();

		void AddToIncomingMessageQueue();

		message<Data> m_temporaryMessageIn;
	};

	template <typename Data>
	Connection<Data>::Connection(Owner owner, asio::io_context& asioContext, asio::ip::tcp::socket socket,
		ThreadSafeQueue<owned_message<Data>>& messageQueue):
		m_owner(owner), m_socket(std::move(socket)), m_asioContext(asioContext), m_messagesIn(messageQueue)
	{
		if (m_owner == Owner::Server)
		{
			m_handShakeOut = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
			m_handShakeCheck = Encrypt(m_handShakeOut);
			m_testPassed = true;
		}
		else
		{
			m_handShakeIn = 0;
			m_handShakeCheck = 0;
		}
	}

	template <typename Data>
	void Connection<Data>::ConnectToServer(const asio::ip::tcp::resolver::results_type& endPoints)
	{
		if (m_owner == Owner::Client)
		{
			asio::async_connect(m_socket, endPoints,
			                    [this](std::error_code errorCode, const asio::ip::tcp::endpoint& endPoint) 
			                    {
				                    if (!errorCode)
				                    {
					                    ReadValidation();
				                    }
			                    });
		}
	}

	template <typename Data>
	void Connection<Data>::Disconnect()
	{
		if (IsConnected())
			asio::post(m_asioContext, [this]() {m_socket.close(); });
	}

	template <typename Data>
	bool Connection<Data>::IsConnected() const
	{
		return m_socket.is_open() && m_testPassed;
	}

	template <typename Data>
	void Connection<Data>::Send(const message<Data>& msg)
	{
		asio::post(m_asioContext, [this, msg]()
		{
			bool alreadyWriting = !m_messagesOut.empty();
			m_messagesOut.push_back(msg);
			if (!alreadyWriting)
			{
				WriteHeader();
			}
		});
	}

	template <typename Data>
	uint32_t Connection<Data>::GetId() const
	{ return m_id; }

	template <typename Data>
	void Connection<Data>::ConnectToClient(ServerInterface<Data>* server, uint32_t id)
	{
		if (m_owner == Owner::Server)
		{
			if (m_socket.is_open())
			{
				m_id = id;
				WriteValidation();
				ReadValidation(server);
			}
		}
	}

	template <typename Data>
	uint64_t Connection<Data>::Encrypt(uint64_t data)
	{
		data = data ^ 0xDEADBEEFC0DECAFE;
		data = (data & 0xF0F0F0F0F0F0F0F0) >> 4 | (data & 0x0F0F0F0F0F0F0F0F) << 4;
		return data ^ 0xC0DEFACE12345678;
	}

	template <typename Data>
	uint64_t Connection<Data>::Decrypt(uint64_t data)
	{
		data = data ^ 0xC0DEFACE12345678;
		data = (data & 0xF0F0F0F0F0F0F0F0) >> 4 | (data & 0x0F0F0F0F0F0F0F0F) << 4;
		return data ^ 0xDEADBEEFC0DECAFE;
	}

	template <typename Data>
	void Connection<Data>::WriteValidation()
	{
		asio::async_write(m_socket, asio::buffer(&m_handShakeOut, sizeof(uint64_t)),
		                  [this](std::error_code errorCode, std::size_t length)
		                  {
			                  if (!errorCode)
			                  {
				                  if (m_owner == Owner::Client)
				                  {
					                  ReadHeader();
					                  m_testPassed = true;
				                  }
			                  }
			                  else
			                  {
				                  m_socket.close();
			                  }
		                  });
	}

	template <typename Data>
	void Connection<Data>::ReadValidation(ServerInterface<Data>* server)
	{
		asio::async_read(m_socket, asio::buffer(&m_handShakeIn, sizeof(uint64_t)),
		                 [this, server](std::error_code errorCode, std::size_t length)
		                 {
			                 if (!errorCode)
			                 {
				                 if (m_owner == Owner::Server)
				                 {
					                 if (m_handShakeIn == m_handShakeCheck)
					                 {
						                 std::cout << "Client Validated" << std::endl;
						                 server->OnClientValidated(this->shared_from_this());
						                 ReadHeader();
					                 }
					                 else
					                 {
						                 std::cout << "Client Disconnected (Fail Validation)" << std::endl;
						                 m_socket.close();
					                 }
				                 }
				                 else
				                 {
					                 m_handShakeOut = Encrypt(m_handShakeIn);
					                 WriteValidation();
				                 }
			                 }
			                 else
			                 {
				                 m_socket.close();
			                 }
		                 });
	}

	template <typename Data>
	void Connection<Data>::ReadHeader()
	{
		asio::async_read(m_socket, asio::buffer(&m_temporaryMessageIn.header, sizeof(message_header<Data>)), 
		                 [this](std::error_code errorCode, std::size_t length)
		                 {
			                 if (!errorCode)
			                 {
				                 if (m_temporaryMessageIn.header.size > 0)
				                 {
					                 m_temporaryMessageIn.body.resize(m_temporaryMessageIn.header.size);
					                 ReadBody();
				                 }
				                 else
				                 {
					                 AddToIncomingMessageQueue();
				                 }
			                 }
			                 else
			                 {
				                 std::cout << "[" << m_id << "] Read Header Fail." << std::endl;
				                 std::cout << errorCode.message() << std::endl;
				                 m_socket.close();
			                 }
		                 });
	}

	template <typename Data>
	void Connection<Data>::ReadBody()
	{
		asio::async_read(m_socket, asio::buffer(m_temporaryMessageIn.body.data(), m_temporaryMessageIn.body.size()), 
		                 [this](std::error_code errorCode, std::size_t length)
		                 {
			                 if (!errorCode)
			                 {
				                 AddToIncomingMessageQueue();
			                 }
			                 else
			                 {
				                 std::cout << "[" << m_id << "] Read Body Fail." << std::endl;
				                 std::cout << errorCode.message() << std::endl;
				                 m_socket.close();
			                 }
		                 });
	}

	template <typename Data>
	void Connection<Data>::WriteHeader()
	{
		asio::async_write(m_socket, asio::buffer(&m_messagesOut.front().header, sizeof(message_header<Data>)),
		                  [this](std::error_code errorCode, std::size_t length)
		                  {
			                  if (!errorCode)
			                  {
				                  if (m_messagesOut.front().body.size() > 0)
				                  {
					                  WriteBody();
				                  }
				                  else
				                  {
					                  m_messagesOut.pop_front();

					                  if (!m_messagesOut.empty())
					                  {
						                  WriteHeader();
					                  }
				                  }
			                  }
		                  });

	}

	template <typename Data>
	void Connection<Data>::WriteBody()
	{
		asio::async_write(m_socket, asio::buffer(&m_messagesOut.front().header, sizeof(message_header<Data>)),
		                  [this](std::error_code errorCode, std::size_t length)
		                  {
			                  if (!errorCode)
			                  {
				                  m_messagesOut.pop_front();

				                  if (!m_messagesOut.empty())
				                  {
					                  WriteHeader();
				                  }
			                  }
			                  else
			                  {
				                  std::cout << "[" << m_id << "] Write Body Fail." << std::endl;
				                  std::cout << errorCode.message() << std::endl;
				                  m_socket.close();
			                  }
		                  });
	}

	template <typename Data>
	void Connection<Data>::AddToIncomingMessageQueue()
	{
		m_messagesIn.push_back({ m_owner == Owner::Server ? this->shared_from_this() : nullptr, m_temporaryMessageIn });

		ReadHeader();
	}
}
