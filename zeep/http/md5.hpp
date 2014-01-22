//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

#include <zeep/config.hpp>
#include <zeep/http/webapp/el.hpp>

// --------------------------------------------------------------------
//

namespace zeep {
namespace http {

class md5
{
  public:
	md5()
	{
		init();
	}

	md5(const void* data, std::size_t length)
	{
		init();
		update(data, length);
	}
	
	md5(const std::string& data)
	{
		init();
		update(data);
	}
	
	void update(const void* data, std::size_t length);
	void update(const std::string& data) { update(data.c_str(), data.length()); }

	std::string finalise();

  private:
	
	void init()
	{
		m_data_length = 0;
		m_bit_length = 0;
		
		m_buffer[0] = 0x67452301;
		m_buffer[1] = 0xefcdab89;
		m_buffer[2] = 0x98badcfe;
		m_buffer[3] = 0x10325476;
	}
	
	void transform(const uint8* data);
	
	uint32 m_buffer[4];
	uint8 m_data[64];
	uint32 m_data_length;
	int64 m_bit_length;
};

}
}
