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
#include <stdexcept>
#include <string>
#include <vector>

//[ cart_items
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

//[ shop_rest_controller
class shop_rest_controller : public zeep::http::controller
{
  public:
	shop_rest_controller()
		: zeep::http::controller("/cart")
	{
		map_post_request("", &shop_rest_controller::create_cart, "client");
		map_get_request("{id}", &shop_rest_controller::get_cart, "id");
		map_post_request("{id}/item", &shop_rest_controller::add_cart_item, "id", "name");
		map_delete_request("{id}/item", &shop_rest_controller::delete_cart_item, "id", "name");
	}

	int create_cart(const std::string &client)
	{
		int cartID = sNextCartID++;
		m_carts.push_back({ .id = cartID, .client = client, .items = {} });
		return cartID;
	}

	Cart &get_cart(int cartID)
	{
		auto oi = std::ranges::find_if(m_carts, [&](auto &o)
			{ return o.id == cartID; });
		if (oi == m_carts.end())
			throw std::invalid_argument("No such cart");
		return *oi;
	}

	Cart add_cart_item(int cartID, const std::string &item)
	{
		Cart &cart = get_cart(cartID);

		auto ii = std::ranges::find_if(cart.items, [&](auto &i)
			{ return i.name == item; });
		if (ii == cart.items.end())
			cart.items.push_back({ item, 1 });
		else
			ii->count += 1;

		return cart;
	}

	Cart delete_cart_item(int cartID, const std::string &item)
	{
		Cart &cart = get_cart(cartID);

		auto ii = std::ranges::find_if(cart.items, [&](auto &i)
			{ return i.name == item; });
		if (ii != cart.items.end())
		{
			if (--ii->count == 0)
				cart.items.erase(ii);
		}

		return cart;
	}

  private:
	static int sNextCartID;
	std::vector<Cart> m_carts;
};
//]

int shop_rest_controller::sNextCartID = 1;

//[ shop_html_controller
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
		return get_template_processor().create_reply_from_template("shop.xhtml", scope);
	}
};
//]

//[ shop_main
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
