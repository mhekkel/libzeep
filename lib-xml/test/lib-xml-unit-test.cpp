#define BOOST_TEST_MODULE Processor_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/xml/node-v2.hpp>
#include <zeep/exception.hpp>

using namespace std;
namespace zx = zeep::xml2;

BOOST_AUTO_TEST_CASE(xml_1)
{
	zx::node n("data", { { "attr1", "value-1" }, { "attr2", "value-2" } });

	BOOST_TEST(n.name() == "data");
	BOOST_TEST(n.attributes().empty() == false);
	BOOST_TEST(n.attributes().size() == 2);
	// BOOST_TEST(n.attributes().begin() != n.attributes().end());

	size_t i = 0;
	for (auto& [name, value]: n.attributes())
	{
		switch (i++)
		{
			case 0:
				BOOST_TEST(name == "attr1");
				BOOST_TEST(value == "value-1");
				break;
			
			case 1:
				BOOST_TEST(name == "attr2");
				BOOST_TEST(value == "value-2");
				break;
		}
	}
}

BOOST_AUTO_TEST_CASE(xml_2)
{
	// zx::node p("data");
	// zx::node n("data-2");

	// p.push_back(n);

	// p.emplace_back("data-3", { { "a", "1"} });

	// BOOST_TEST(n.children().empty() == false);
	// BOOST_TEST(n.children().size() == 2);
	// BOOST_TEST(n.children().begin() != n.children().end());

	// size_t i = 0;
	// for (auto& child: n.children())
	// {
	// 	switch (i++)
	// 	{
	// 		case 0:
	// 			BOOST_TEST(child.name() == "data-2");
	// 			BOOST_TEST(child.children().empty() == true);
	// 			BOOST_TEST(child.children().size() == 0);
	// 			BOOST_TEST(child.children().begin() == child.children().end());
	// 			BOOST_TEST(child.attributes().empty() == true);
	// 			BOOST_TEST(child.attributes().size() == 0);
	// 			BOOST_TEST(child.attributes().begin() == child.attributes().end());
	// 			break;
			
	// 		case 1:
	// 			BOOST_TEST(child.name() == "data-3");
	// 			BOOST_TEST(child.children().empty() == true);
	// 			BOOST_TEST(child.children().size() == 0);
	// 			BOOST_TEST(child.children().begin() == child.children().end));
	// 			BOOST_TEST(child.attributes().empty() == false);
	// 			BOOST_TEST(child.attributes().size() == 1);
	// 			BOOST_TEST(child.attributes().begin() != child.attributes().end());
	// 			break;
	// 	}
	// }
}