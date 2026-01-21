//         Copyright Maarten L. Hekkelman, 2022-2026
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES
#define WEBAPP_USES_RESOURCES 0

#include <zeep/http/controller.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/reply.hpp>
#include <zeep/http/server.hpp>
#include <zeep/http/template-processor.hpp>

#include <zeem/serialize.hpp>

#include <cstdint>
#include <exception>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

//[ cart_items_2
struct Item
{
	std::string name;
	uint32_t count;

	template <typename Archive>
	void serialize(Archive &ar, uint64_t /*version*/)
	{
		// clang-format off
		ar & zeem::name_value_pair("name", name)
		   & zeem::name_value_pair("count", count);
		// clang-format on
	}
};

struct Cart
{
	int id;
	std::string client;
	std::vector<Item> items;

	template <typename Archive>
	void serialize(Archive &ar, uint64_t /*version*/)
	{
		// clang-format off
		ar & zeem::name_value_pair("id", id)
		   & zeem::name_value_pair("client", client)
		   & zeem::name_value_pair("items", items);
		// clang-format on
	}
};
//]

//[ shop_rest_controller_2
class shop_rest_controller : public zeep::http::controller
{
  public:
	shop_rest_controller()
		: zeep::http::controller("/cart")
	{
		// CRUD example interface

		map_post_request("", &shop_rest_controller::create_cart, "cart");
		map_get_request("{id}", &shop_rest_controller::retrieve_cart, "id");
		map_put_request("{id}", &shop_rest_controller::update_cart, "id", "cart");
		map_delete_request("{id}", &shop_rest_controller::delete_cart, "id");
	}

	int create_cart(Cart cart)
	{
		int result = cart.id = sNextCartID++;
		m_carts.push_back(std::move(cart));
		return result;
	}

	Cart &retrieve_cart(int cartID)
	{
		auto oi = std::ranges::find_if(m_carts, [&](auto &o)
			{ return o.id == cartID; });
		if (oi == m_carts.end())
			throw std::invalid_argument("No such cart");
		return *oi;
	}

	void update_cart(int cartID, const Cart &cart)
	{
		auto oi = std::ranges::find_if(m_carts, [&](auto &o)
			{ return o.id == cartID; });
		if (oi == m_carts.end())
			throw std::invalid_argument("No such cart");

		oi->client = cart.client;
		oi->items = cart.items;
	}

	void delete_cart(int cartID)
	{
		std::erase_if(m_carts, [cartID](auto &cart)
			{ return cart.id == cartID; });
	}

  private:
	static int sNextCartID;
	std::vector<Cart> m_carts;
};
//]

int shop_rest_controller::sNextCartID = 1;

//[ shop_html_controller_2
class shop_html_controller : public zeep::http::html_controller
{
  public:
	shop_html_controller()
	{
		map_get("", &shop_html_controller::handle_index);
		map_get_file("{css,scripts}/");
	}

	zeep::http::reply handle_index(const zeep::http::scope &scope)
	{
		return get_template_processor().create_reply_from_template("shop-2.xhtml", scope);
	}
};
//]

//[ shop_main_2
int main()
{
	try
	{
		/* Use the server constructor that takes the path to a docroot so it will construct a template processor */
		zeep::http::server srv("docroot");

		srv.add_controller(new shop_html_controller());
		srv.add_controller(new shop_rest_controller());

		srv.bind("::", 8080);

		// Note that the rest controller above is not thread safe!
		srv.run(1);
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << '\n';
	}

	return 0;
}
//]
