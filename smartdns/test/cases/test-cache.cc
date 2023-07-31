/*************************************************************************
 *
 * Copyright (C) 2018-2023 Ruilin Peng (Nick) <pymumu@gmail.com>.
 *
 * smartdns is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * smartdns is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "client.h"
#include "dns.h"
#include "include/utils.h"
#include "server.h"
#include "gtest/gtest.h"
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

/* clang-format off */
#include "dns_cache.h"
/* clang-format on */

class Cache : public ::testing::Test
{
  protected:
	void SetUp() override {}

	void TearDown() override {}
};

TEST_F(Cache, min)
{
	smartdns::MockServer server_upstream;
	smartdns::Server server;

	server_upstream.Start("udp://0.0.0.0:61053", [](struct smartdns::ServerRequestContext *request) {
		std::string domain = request->domain;
		if (request->domain.length() == 0) {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		if (request->qtype == DNS_T_A) {
			unsigned char addr[4] = {1, 2, 3, 4};
			dns_add_A(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else if (request->qtype == DNS_T_AAAA) {
			unsigned char addr[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			dns_add_AAAA(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		request->response_packet->head.rcode = DNS_RC_NOERROR;
		return smartdns::SERVER_REQUEST_OK;
	});

	server.Start(R"""(bind [::]:60053
server 127.0.0.1:61053
log-num 0
cache-size 1
rr-ttl-min 1
speed-check-mode none
response-mode fastest-response
log-console yes
log-level debug
cache-persist no)""");
	smartdns::Client client;
	ASSERT_TRUE(client.Query("a.com", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetName(), "a.com");
	EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 1);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
}

TEST_F(Cache, max_reply_ttl)
{
	smartdns::MockServer server_upstream;
	smartdns::Server server;

	server_upstream.Start("udp://0.0.0.0:61053", [](struct smartdns::ServerRequestContext *request) {
		std::string domain = request->domain;
		if (request->domain.length() == 0) {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		if (request->qtype == DNS_T_A) {
			unsigned char addr[4] = {1, 2, 3, 4};
			dns_add_A(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else if (request->qtype == DNS_T_AAAA) {
			unsigned char addr[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			dns_add_AAAA(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		request->response_packet->head.rcode = DNS_RC_NOERROR;
		return smartdns::SERVER_REQUEST_OK;
	});

	server.Start(R"""(bind [::]:60053
server 127.0.0.1:61053
log-num 0
cache-size 1
rr-ttl-min 600
rr-ttl-reply-max 5
speed-check-mode none
response-mode fastest-response
log-console yes
log-level debug
cache-persist no)""");
	smartdns::Client client;
	ASSERT_TRUE(client.Query("a.com", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetName(), "a.com");
	EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 5);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
}

TEST_F(Cache, max_reply_ttl_expired)
{
	smartdns::MockServer server_upstream;
	smartdns::Server server;

	server_upstream.Start("udp://0.0.0.0:61053", [](struct smartdns::ServerRequestContext *request) {
		std::string domain = request->domain;
		if (request->domain.length() == 0) {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		if (request->qtype == DNS_T_A) {
			unsigned char addr[4] = {1, 2, 3, 4};
			dns_add_A(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else if (request->qtype == DNS_T_AAAA) {
			unsigned char addr[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			dns_add_AAAA(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		request->response_packet->head.rcode = DNS_RC_NOERROR;
		return smartdns::SERVER_REQUEST_OK;
	});

	server.Start(R"""(bind [::]:60053
server 127.0.0.1:61053
log-num 0
cache-size 1
rr-ttl-min 600
rr-ttl-reply-max 6
log-console yes
log-level debug
cache-persist no)""");
	smartdns::Client client;
	ASSERT_TRUE(client.Query("a.com", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetName(), "a.com");
	EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");

	ASSERT_TRUE(client.Query("a.com", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetName(), "a.com");
	EXPECT_GE(client.GetAnswer()[0].GetTTL(), 5);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
}

TEST_F(Cache, nocache)
{
	smartdns::MockServer server_upstream;
	smartdns::Server server;

	server_upstream.Start("udp://0.0.0.0:61053", [](struct smartdns::ServerRequestContext *request) {
		std::string domain = request->domain;
		if (request->domain.length() == 0) {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		usleep(15000);

		if (request->qtype == DNS_T_A) {
			unsigned char addr[4] = {1, 2, 3, 4};
			dns_add_A(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else if (request->qtype == DNS_T_AAAA) {
			unsigned char addr[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
			dns_add_AAAA(request->response_packet, DNS_RRS_AN, domain.c_str(), 0, addr);
		} else {
			return smartdns::SERVER_REQUEST_ERROR;
		}

		request->response_packet->head.rcode = DNS_RC_NOERROR;
		return smartdns::SERVER_REQUEST_OK;
	});

	server.Start(R"""(bind [::]:60053
server 127.0.0.1:61053
log-num 0
cache-size 100
rr-ttl-min 600
rr-ttl-reply-max 5
log-console yes
log-level debug
domain-rules /a.com/ --no-cache
cache-persist no)""");
	smartdns::Client client;
	ASSERT_TRUE(client.Query("a.com", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetName(), "a.com");
	EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");

	ASSERT_TRUE(client.Query("a.com", 60053));
	EXPECT_GT(client.GetQueryTime(), 10);
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetName(), "a.com");
	EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
}

TEST_F(Cache, save_file)
{
	smartdns::MockServer server_upstream;
	auto cache_file = "/tmp/smartdns_cache." + smartdns::GenerateRandomString(10);
	std::string conf = R"""(
bind [::]:60053@lo
server 127.0.0.1:62053
log-num 0
log-console yes
log-level debug
cache-persist yes
dualstack-ip-selection no
)""";

	conf += "cache-file " + cache_file;
	Defer
	{
		unlink(cache_file.c_str());
	};

	server_upstream.Start("udp://0.0.0.0:62053", [](struct smartdns::ServerRequestContext *request) {
		if (request->qtype == DNS_T_A) {
			smartdns::MockServer::AddIP(request, request->domain.c_str(), "1.2.3.4");
			return smartdns::SERVER_REQUEST_OK;
		}
		return smartdns::SERVER_REQUEST_SOA;
	});
	{
		smartdns::Server server;
		server.Start(conf);
		smartdns::Client client;

		ASSERT_TRUE(client.Query("a.com", 60053));
		std::cout << client.GetResult() << std::endl;
		ASSERT_EQ(client.GetAnswerNum(), 1);
		EXPECT_EQ(client.GetStatus(), "NOERROR");
		EXPECT_LT(client.GetQueryTime(), 100);
		EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
		EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
		server.Stop();
		usleep(200 * 1000);
	}

	ASSERT_EQ(access(cache_file.c_str(), F_OK), 0);

	std::fstream fs(cache_file, std::ios::in);
	struct dns_cache_file head;
	memset(&head, 0, sizeof(head));
	fs.read((char *)&head, sizeof(head));
	EXPECT_EQ(head.magic, MAGIC_NUMBER);
	EXPECT_EQ(head.cache_number, 1);
}

TEST_F(Cache, corrupt_file)
{
	smartdns::MockServer server_upstream;
	auto cache_file = "/tmp/smartdns_cache." + smartdns::GenerateRandomString(10);
	std::string conf = R"""(
bind [::]:60053@lo
server 127.0.0.1:62053
log-num 0
log-console yes
log-level debug
dualstack-ip-selection no
cache-persist yes
)""";

	conf += "cache-file " + cache_file;
	Defer
	{
		unlink(cache_file.c_str());
	};

	server_upstream.Start("udp://0.0.0.0:62053", [](struct smartdns::ServerRequestContext *request) {
		if (request->qtype == DNS_T_A) {
			smartdns::MockServer::AddIP(request, request->domain.c_str(), "1.2.3.4");
			return smartdns::SERVER_REQUEST_OK;
		}
		return smartdns::SERVER_REQUEST_SOA;
	});
	{
		smartdns::Server server;
		server.Start(conf);
		smartdns::Client client;

		ASSERT_TRUE(client.Query("a.com", 60053));
		std::cout << client.GetResult() << std::endl;
		ASSERT_EQ(client.GetAnswerNum(), 1);
		EXPECT_EQ(client.GetStatus(), "NOERROR");
		EXPECT_LT(client.GetQueryTime(), 100);
		EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
		EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
		server.Stop();
		usleep(200 * 1000);
	}

	ASSERT_EQ(access(cache_file.c_str(), F_OK), 0);

	int fd = open(cache_file.c_str(), O_RDWR);
	ASSERT_NE(fd, -1);
	srandom(time(NULL));
	off_t file_size = lseek(fd, 0, SEEK_END);
	off_t offset = random() % (file_size - 300);
	std::cout << "try make corrupt at " << offset << ", file size: " << file_size << std::endl;
	lseek(fd, offset, SEEK_SET);
	for (int i = 0; i < 300; i++) {
		unsigned char c = random() % 256;
		write(fd, &c, 1);
	}
	close(fd);
	{
		smartdns::Server server;
		server.Start(conf);
		smartdns::Client client;

		ASSERT_TRUE(client.Query("a.com", 60053));
		std::cout << client.GetResult() << std::endl;
		ASSERT_EQ(client.GetAnswerNum(), 1);
		EXPECT_EQ(client.GetStatus(), "NOERROR");
		EXPECT_LT(client.GetQueryTime(), 100);
		EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
		EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
		server.Stop();
		usleep(200 * 1000);
	}
}