// SPDX-FileCopyrightText: Maarten L. Hekkelman 2021.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <thread>

namespace zeep
{

class signal_catcher
{
  public:
	signal_catcher(const signal_catcher &) = delete;
	signal_catcher &operator=(const signal_catcher &) = delete;

	signal_catcher();
	~signal_catcher();

	void block();
	void unblock();

	int wait();

	static void signal_hangup(std::thread &t);

  private:
	struct signal_catcher_impl *mImpl;
};

} // namespace zeep
