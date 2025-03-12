#include "gtest/gtest.h"
#include "CommonIncludes.h"
#include "ClientInterface.hpp"
#include "ServerInterface.hpp"
#include "Connection.hpp"
#include "Message.hpp"
#include "ThreadSafeQueue.hpp"

TEST(CommonTest, Encrypt)
{
	const uint64_t random = std::chrono::system_clock::now().time_since_epoch().count();
	const uint64_t encrypted = sockets::connection<uint64_t>::Encrypt(random);
	const uint64_t decrypted = sockets::connection<uint64_t>::Decrypt(encrypted);
	EXPECT_EQ(random, decrypted);
}


int RunAllTests()
{
	testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}
