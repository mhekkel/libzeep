// SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
//
// SPDX-License-Identifier: BSL-1.0

#include <chrono>
#include <iostream>

int main()
{
	auto now = std::chrono::system_clock::now();
	auto t = std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(now) };
	std::cout << std::format("{:%d/%b/%Y:%H:%M:%S %Ez}", t);

	return 0;
}