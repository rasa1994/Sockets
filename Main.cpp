#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include <vector>
#include <thread>

std::vector<char> buffer(20 * 1024);

void GrabData(asio::ip::tcp::socket& socket)
{
	socket.async_read_some(asio::buffer(buffer.data(), buffer.size()), [&](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				std::cout << "\n\nRead " << length << " bytes\n\n";
				for (int i = 0; i < length; i++)
				{
					std::cout << buffer[i];
				}
				GrabData(socket);
			}
		});
}

int main()
{
	asio::error_code code;



	asio::io_context context;

	asio::io_context::work idleWork(context);

	std::jthread threadContext = std::jthread([&](std::stop_token token)
		{
			context.run();
		});

	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("96.7.128.175", code), 80);

	asio::ip::tcp::socket socket(context);

	socket.connect(endpoint, code);

	if (!code)
	{
		std::cout << "Connected!" << std::endl;
	}
	else
	{
		std::cout << "Failed to connect to address:\n" << code.message() << std::endl;
	}

	if (socket.is_open())
	{
		GrabData(socket);

		std::string request = "GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n\r\n";
		socket.write_some(asio::buffer(request.data(), request.size()), code);
		std::this_thread::sleep_for(std::chrono::seconds(20));
	}

	system("pause");
}