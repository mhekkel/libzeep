//           Copyright Maarten L. Hekkelman, 2022
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/http/soap-controller.hpp>

//[ cart_items
struct Item
{
    std::string name;
    uint32_t	count;

    template<typename Archive>
    void serialize(Archive& ar, unsigned long version)
    {
        ar & zeep::make_nvp("name", name)
           & zeep::make_nvp("count", count);
    }
};

struct Cart
{
    int					id;
    std::string			client;
    std::vector<Item>	items;

    template<typename Archive>
    void serialize(Archive& ar, unsigned long version)
    {
        ar & zeep::make_nvp("id", id)
           & zeep::make_nvp("client", client)
           & zeep::make_nvp("items", items);
    }
};
//]

//[ shop_soap_controller
class shop_soap_controller : public zeep::http::soap_controller
{
  public:
    shop_soap_controller()
        : zeep::http::soap_controller("/ws", "cart", "https://www.hekkelman.com/libzeep/soap-sample")
    {
        map_action("create", &shop_soap_controller::create_cart, "client");
        map_action("retrieve", &shop_soap_controller::get_cart, "id");
        map_action("update", &shop_soap_controller::add_cart_item, "id", "name");
        map_action("delete", &shop_soap_controller::delete_cart_item, "id", "name");
    }

    int create_cart(const std::string& client)
    {
        int cartID = sNextCartID++;
        m_carts.push_back({ cartID, client });
        return cartID;
    }

    Cart get_cart(int cartID)
    {
        auto oi = std::find_if(m_carts.begin(), m_carts.end(), [&](auto& o) { return o.id == cartID; });
        if (oi == m_carts.end())
            throw std::invalid_argument("No such cart");
        return *oi;
    }

    Cart add_cart_item(int cartID, const std::string& item)
    {
        auto oi = std::find_if(m_carts.begin(), m_carts.end(), [&](auto& o) { return o.id == cartID; });
        if (oi == m_carts.end())
            throw std::invalid_argument("No such cart");

        Cart &cart = *oi;

        auto ii = std::find_if(cart.items.begin(), cart.items.end(), [&](auto& i) { return i.name == item; });
        if (ii == cart.items.end())
            cart.items.push_back({item, 1});
        else
            ii->count += 1;

        return cart;
    }

    Cart delete_cart_item(int cartID, const std::string& item)
    {
        auto oi = std::find_if(m_carts.begin(), m_carts.end(), [&](auto& o) { return o.id == cartID; });
        if (oi == m_carts.end())
            throw std::invalid_argument("No such cart");

        Cart &cart = *oi;

        auto ii = std::find_if(cart.items.begin(), cart.items.end(), [&](auto& i) { return i.name == item; });
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

int shop_soap_controller::sNextCartID = 1;

//[ shop_main
int main()
{
    /*<< Use the server constructor that takes the path to a docroot so it will construct a template processor >>*/
    zeep::http::server srv("docroot");

    srv.add_controller(new shop_soap_controller());

    srv.bind("::", 8080);

    // Note that the rest controller above is not thread safe!
    srv.run(1);

    return 0;
}
//]