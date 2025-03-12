#pragma once 

#include "CommonIncludes.h"
#include "ThreadSafeQueue.hpp"
#include "Message.hpp"

namespace sockets
{
	template <typename Data>
	class ServerInterface;

	template <typename Data>
	class connection : public std::enable_shared_from_this<connection<Data>>
	{
	public:
		enum class Owner : uint8_t
		{
			Server,
			Client
		};


		connection(Owner owner, asio::io_context& asioContext, asio::ip::tcp::socket socket, ThreadSafeQueue<sockets::owned_message<Data>>& messageQueue) :
			m_asioContext(asioContext), m_socket(std::move(socket)), m_messagesIn(messageQueue), m_owner(owner)
		{
			if (m_owner == Owner::Server)
			{
				m_handShakeOut = uint32_t(std::chrono::system_clock::now().time_since_epoch().count());
				m_handShakeCheck = Encrypt(m_handShakeOut);
			}
			{
				m_handShakeIn = 0;
				m_handShakeCheck = 0;
			}
		}

		virtual ~connection()
		{

		}

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endPoints)
		{
			if (m_owner == Owner::Client)
			{
				asio::async_connect(m_socket, endPoints,
					[this](std::error_code errorCode, asio::ip::tcp::endpoint endPoint) 
					{
						if (!errorCode)
						{
							ReadValidation();
						}
					});
			}
		}

		void Disconnect()
		{
			if (IsConnected())
				asio::post(m_asioContext, [this]() {m_socket.close(); });
		}
		
		
		bool IsConnected() const
		{
			return m_socket.is_open();
		}


		bool Send(const message<Data>& msg)
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
		uint32_t GetId() const { return m_id; }

		void ConnectToClient(sockets::ServerInterface<Data>* server, uint32_t id = 0)
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

		uint64_t Encrypt(uint64_t data)
		{
			auto result = data ^ 0xBAADCAFEBAADCAFE;
			result = (result & 0x00FF00FF00FF00FF) << 8 | (result & 0x00FF00FF00FF00FF) >> 8;
			return result ^ 0xC0DEFACE12345678;
		}

		void WriteValidation()
		{
			asio::async_write(m_socket, asio::buffer(&m_handShakeOut, sizeof(uint32_t)),
				[this](std::error_code errorCode, std::size_t length)
				{
					if (!errorCode)
					{
						if (m_owner == Owner::Client)
							ReadHeader();
					}
					else
					{
						m_socket.close();
					}
				});
		}

		void ReadValidation(sockets::ServerInterface<Data>* server = nullptr)
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

	protected:
		uint32_t m_id{ 0 };
		Owner m_owner = Owner::Server;
		asio::ip::tcp::socket m_socket;
		asio::io_context& m_asioContext;
		ThreadSafeQueue<message<Data>> m_messagesOut;
		ThreadSafeQueue<owned_message<Data>>& m_messagesIn;

		uint32_t m_handShakeOut{ 0 };
		uint32_t m_handShakeIn{ 0 };
		uint32_t m_handShakeCheck{ 0 };

	private:
		void ReadHeader()
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
						m_socket.close();
					}
				});
		}

		void ReadBody()
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
						m_socket.close();
					}
				});
		}

		void WriteHeader()
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

		void WriteBody()
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
						m_socket.close();
					}
				});
		}

		void AddToIncomingMessageQueue()
		{
			m_messagesIn.push_back({ m_owner == Owner::Server ? this->shared_from_this() : nullptr, m_temporaryMessageIn });

			ReadHeader();
		}

		message<Data> m_temporaryMessageIn;
	};
}