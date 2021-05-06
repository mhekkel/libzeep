//          Copyright Maarten L. Hekkelman 2021.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <thread>

namespace zeep
{

class signal_catcher
{
public:
	signal_catcher();
	~signal_catcher();

	void block();
	void unblock();

	int wait();

	static void signal_hangup(std::thread& t);

private:
	signal_catcher(const signal_catcher &) = delete;
	signal_catcher &operator=(const signal_catcher &) = delete;

	struct signal_catcher_impl *mImpl;
};

}
