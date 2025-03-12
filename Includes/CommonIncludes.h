#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <functional>
#include <algorithm>
#include <chrono>
#include <type_traits>
#include <iostream>
#include <ranges>
#include <utility>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>