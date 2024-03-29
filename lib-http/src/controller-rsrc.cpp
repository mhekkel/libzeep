// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/http/template-processor.hpp>
#include <zeep/streambuf.hpp>

namespace fs = std::filesystem;

// --------------------------------------------------------------------
// We have a special, private version of mrsrc here. To be able to create
// shared libraries and still be able to link when there's no mrc used.

namespace mrsrc
{
/// \brief Internal data structure as generated by mrc
struct rsrc_imp
{
	unsigned int m_next;
	unsigned int m_child;
	unsigned int m_name;
	unsigned int m_size;
	unsigned int m_data;
};
} // namespace mrsrc

#if _MSC_VER

extern "C" const mrsrc::rsrc_imp *gResourceIndexDefault[1] = {};
extern "C" const char *gResourceDataDefault[1] = {};
extern "C" const char *gResourceNameDefault[1] = {};

extern "C" const mrsrc::rsrc_imp gResourceIndex[];
extern "C" const char gResourceData[];
extern "C" const char gResourceName[];

#pragma comment(linker, "/alternatename:gResourceIndex=gResourceIndexDefault")
#pragma comment(linker, "/alternatename:gResourceData=gResourceDataDefault")
#pragma comment(linker, "/alternatename:gResourceName=gResourceNameDefault")

#else

extern "C" __attribute__((weak, alias("gResourceIndexDefault"))) const mrsrc::rsrc_imp gResourceIndex[1];
extern "C" __attribute__((weak, alias("gResourceDataDefault"))) const char gResourceData[1];
extern "C" __attribute__((weak, alias("gResourceNameDefault"))) const char gResourceName[1];

const mrsrc::rsrc_imp *gResourceIndexDefault[1] = {};
const char *gResourceDataDefault[1] = {};
const char *gResourceNameDefault[1] = {};

#endif

namespace mrsrc
{
class rsrc_data
{
  public:
	static rsrc_data &instance()
	{
		static rsrc_data s_instance;
		return s_instance;
	}

	const rsrc_imp *index() const { return m_index; }

	const char *data(unsigned int offset) const
	{
		return m_data + offset;
	}

	const char *name(unsigned int offset) const
	{
		return m_name + offset;
	}

  private:
	rsrc_data()
	{
		m_index = gResourceIndex;
		m_data = gResourceData;
		m_name = gResourceName;
	}

	rsrc_imp m_dummy = {};
	const rsrc_imp *m_index = &m_dummy;
	const char *m_data = "";
	const char *m_name = "";
};

/// \brief Class mrsrc::rsrc contains a pointer to the data in the
/// resource, as well as offering an iterator interface to its
/// children.

class rsrc
{
  public:
	rsrc()
		: m_impl(rsrc_data::instance().index())
	{
	}

	rsrc(const rsrc &other)
		: m_impl(other.m_impl)
	{
	}

	rsrc &operator=(const rsrc &other)
	{
		m_impl = other.m_impl;
		return *this;
	}

	rsrc(std::filesystem::path path);

	std::string name() const { return m_impl ? rsrc_data::instance().name(m_impl->m_name) : ""; }

	const char *data() const { return m_impl ? rsrc_data::instance().data(m_impl->m_data) : nullptr; }

	unsigned long size() const { return m_impl ? m_impl->m_size : 0; }

	explicit operator bool() const { return m_impl != NULL and m_impl->m_size > 0; }

	template <typename RSRC>
	class iterator_t
	{
	  public:
		using iterator_category = std::input_iterator_tag;
		using value_type = RSRC;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		iterator_t(const rsrc_imp *cur)
			: m_cur(cur)
		{
		}

		iterator_t(const iterator_t &i)
			: m_cur(i.m_cur)
		{
		}

		iterator_t &operator=(const iterator_t &i)
		{
			m_cur = i.m_cur;
			return *this;
		}

		reference operator*() { return m_cur; }
		pointer operator->() { return &m_cur; }

		iterator_t &operator++()
		{
			if (m_cur.m_impl->m_next)
				m_cur.m_impl = rsrc_data::instance().index() + m_cur.m_impl->m_next;
			else
				m_cur.m_impl = nullptr;
			return *this;
		}

		iterator_t operator++(int)
		{
			auto tmp(*this);
			this->operator++();
			return tmp;
		}

		bool operator==(const iterator_t &rhs) const { return m_cur.m_impl == rhs.m_cur.m_impl; }
		bool operator!=(const iterator_t &rhs) const { return m_cur.m_impl != rhs.m_cur.m_impl; }

	  private:
		value_type m_cur;
	};

	using iterator = iterator_t<rsrc>;

	iterator begin() const
	{
		const rsrc_imp *impl = nullptr;
		if (m_impl and m_impl->m_child)
			impl = rsrc_data::instance().index() + m_impl->m_child;
		return iterator(impl);
	}

	iterator end() const
	{
		return iterator(nullptr);
	}

  private:
	rsrc(const rsrc_imp *imp)
		: m_impl(imp)
	{
	}

	const rsrc_imp *m_impl;
};

inline rsrc::rsrc(std::filesystem::path p)
{
	m_impl = rsrc_data::instance().index();

	// using std::filesytem::path would have been natural here of course...

	auto pb = p.begin();
	auto pe = p.end();

	while (m_impl != nullptr and pb != pe)
	{
		auto name = *pb++;

		const rsrc_imp *impl = nullptr;
		for (rsrc child : *this)
		{
			if (child.name() == name)
			{
				impl = child.m_impl;
				break;
			}
		}

		m_impl = impl;
	}

	if (pb != pe) // not found
		m_impl = nullptr;
}

// --------------------------------------------------------------------

template <typename CharT, typename Traits>
class basic_streambuf : public std::basic_streambuf<CharT, Traits>
{
  public:
	typedef CharT char_type;
	typedef Traits traits_type;
	typedef typename traits_type::int_type int_type;
	typedef typename traits_type::pos_type pos_type;
	typedef typename traits_type::off_type off_type;

	/// \brief constructor taking a \a path to the resource in memory
	basic_streambuf(const std::string &path)
		: m_rsrc(path)
	{
		init();
	}

	/// \brief constructor taking a \a rsrc
	basic_streambuf(const rsrc &rsrc)
		: m_rsrc(rsrc)
	{
		init();
	}

	basic_streambuf(const basic_streambuf &) = delete;

	basic_streambuf(basic_streambuf &&rhs)
		: basic_streambuf(rhs.m_rsrc)
	{
	}

	basic_streambuf &operator=(const basic_streambuf &) = delete;

	basic_streambuf &operator=(basic_streambuf &&rhs)
	{
		swap(rhs);
		return *this;
	}

	void swap(basic_streambuf &rhs)
	{
		std::swap(m_begin, rhs.m_begin);
		std::swap(m_end, rhs.m_end);
		std::swap(m_current, rhs.m_current);
	}

	/// \brief Analogous to is_open of an ifstream_buffer, return true if the resource is valid
	bool is_valid() const
	{
		return static_cast<bool>(m_rsrc);
	}

  private:
	void init()
	{
		if (m_rsrc)
		{
			m_begin = reinterpret_cast<const char_type *>(m_rsrc.data());
			m_end = reinterpret_cast<const char_type *>(m_rsrc.data() + m_rsrc.size());
			m_current = m_begin;
		}
	}

	int_type underflow()
	{
		if (m_current == m_end)
			return traits_type::eof();

		return traits_type::to_int_type(*m_current);
	}

	int_type uflow()
	{
		if (m_current == m_end)
			return traits_type::eof();

		return traits_type::to_int_type(*m_current++);
	}

	int_type pbackfail(int_type ch)
	{
		if (m_current == m_begin or (ch != traits_type::eof() and ch != m_current[-1]))
			return traits_type::eof();

		return traits_type::to_int_type(*--m_current);
	}

	std::streamsize showmanyc()
	{
		assert(std::less_equal<const char *>()(m_current, m_end));
		return m_end - m_current;
	}

	pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode /*which*/)
	{
		switch (dir)
		{
			case std::ios_base::beg:
				m_current = m_begin + off;
				break;

			case std::ios_base::end:
				m_current = m_end + off;
				break;

			case std::ios_base::cur:
				m_current += off;
				break;

			default:
				break;
		}

		if (m_current < m_begin)
			m_current = m_begin;

		if (m_current > m_end)
			m_current = m_end;

		return m_current - m_begin;
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode /*which*/)
	{
		m_current = m_begin + pos;

		if (m_current < m_begin)
			m_current = m_begin;

		if (m_current > m_end)
			m_current = m_end;

		return m_current - m_begin;
	}

  private:
	rsrc m_rsrc;
	const char_type *m_begin = nullptr;
	const char_type *m_end = nullptr;
	const char_type *m_current = nullptr;
};

using streambuf = basic_streambuf<char, std::char_traits<char>>;

// --------------------------------------------------------------------
// class mrsrc::istream

template <typename CharT, typename Traits>
class basic_istream : public std::basic_istream<CharT, Traits>
{
  public:
	typedef CharT char_type;
	typedef Traits traits_type;
	typedef typename traits_type::int_type int_type;
	typedef typename traits_type::pos_type pos_type;
	typedef typename traits_type::off_type off_type;

  private:
	using __streambuf_type = basic_streambuf<CharT, Traits>;
	using __istream_type = std::basic_istream<CharT, Traits>;

	__streambuf_type m_buffer;

  public:
	basic_istream(const std::string &path)
		: basic_istream(rsrc(path))
	{
	}

	basic_istream(const rsrc &resource)
		: __istream_type(&m_buffer)
		, m_buffer(resource)
	{
		if (resource)
			this->init(&m_buffer);
		else
			__istream_type::setstate(std::ios_base::badbit);
	}

	basic_istream(const basic_istream &) = delete;

	basic_istream(basic_istream &&rhs)
		: __istream_type(std::move(rhs))
		, m_buffer(std::move(rhs.m_buffer))
	{
		__istream_type::set_rdbuf(&m_buffer);
	}

	basic_istream &operator=(const basic_istream &) = delete;

	basic_istream &operator=(basic_istream &&rhs)
	{
		__istream_type::operator=(std::move(rhs));
		m_buffer = std::move(rhs.m_buffer);
		return *this;
	}

	void swap(basic_istream &rhs)
	{
		__istream_type::swap(rhs);
		m_buffer.swap(rhs.m_buffer);
	}

	__streambuf_type *rdbuf() const
	{
		return const_cast<__streambuf_type *>(&m_buffer);
	}
};

using istream = basic_istream<char, std::char_traits<char>>;
} // namespace mrsrc

// --------------------------------------------------------------------

namespace zeep::http
{

// -----------------------------------------------------------------------

#if _WIN32 and not defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

rsrc_loader::rsrc_loader(const std::string &)
{
#if _WIN32
	char exePath[MAX_PATH] = {};
	if (::GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0)
		mRsrcWriteTime = fs::last_write_time(exePath);
#else
	char exePath[PATH_MAX + 1];
	int r = readlink("/proc/self/exe", exePath, PATH_MAX);
	if (r > 0)
	{
		exePath[r] = 0;
		mRsrcWriteTime = fs::last_write_time(exePath);
	}
#endif
}

/// return last_write_time of \a file
fs::file_time_type rsrc_loader::file_time(const std::string &file, std::error_code &ec) noexcept
{
	fs::file_time_type result = {};

	ec = {};
	mrsrc::rsrc rsrc(file);

	if (rsrc)
		result = mRsrcWriteTime;
	else
		ec = std::make_error_code(std::errc::no_such_file_or_directory);

	return result;
}

// basic loader, returns error in ec if file was not found
std::istream *rsrc_loader::load_file(const std::string &file, std::error_code &ec) noexcept
{
	mrsrc::rsrc resource(file);

	std::istream *result = nullptr;
	ec = {};

	if (resource)
		result = new mrsrc::istream(resource);
	else
		ec = std::make_error_code(std::errc::no_such_file_or_directory);

	return result;
}

} // namespace zeep::http
