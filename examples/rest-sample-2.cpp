// In this example we don't want to use rsrc based templates
#undef WEBAPP_USES_RESOURCES

#include <zeep/http/rest-controller.hpp>
#include <zeep/http/html-controller.hpp>

//[ cart_items_2
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

//[ shop_rest_controller_2
class shop_rest_controller : public zeep::http::rest_controller
{
  public:
    shop_rest_controller()
        : zeep::http::rest_controller("/cart")
    {
        // CRUD example interface

        map_post_request("",        &shop_rest_controller::create_cart, "cart");
        map_get_request("{id}",     &shop_rest_controller::retrieve_cart, "id");
        map_put_request("{id}",     &shop_rest_controller::update_cart, "id", "cart");
        map_delete_request("{id}",  &shop_rest_controller::delete_cart, "id");
    }

    int create_cart(Cart cart)
    {
        int result = cart.id = sNextCartID++;
        m_carts.push_back(std::move(cart));
        return result;
    }

    Cart& retrieve_cart(int cartID)
    {
        auto oi = std::find_if(m_carts.begin(), m_carts.end(), [&](auto& o) { return o.id == cartID; });
        if (oi == m_carts.end())
            throw std::invalid_argument("No such cart");
        return *oi;
    }

    void update_cart(int cartID, const Cart& cart)
    {
        auto oi = std::find_if(m_carts.begin(), m_carts.end(), [&](auto& o) { return o.id == cartID; });
        if (oi == m_carts.end())
            throw std::invalid_argument("No such cart");

        oi->client = cart.client;
        oi->items = cart.items;
    }

    void delete_cart(int cartID)
    {
        m_carts.erase(std::remove_if(m_carts.begin(), m_carts.end(), [cartID](auto& cart) { return cart.id == cartID; }), m_carts.end());
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
        mount("", &shop_html_controller::handle_index);
        mount("{css,scripts}/", &shop_html_controller::handle_file);
    }

    void handle_index(const zeep::http::request& req, const zeep::http::scope& scope, zeep::http::reply& rep)
    {
        get_template_processor().create_reply_from_template("shop-2.xhtml", scope, rep);
    }
};
//]

//[ shop_main_2
int main()
{
    /*<< Use the server constructor that takes the path to a docroot so it will construct a template processor >>*/
    zeep::http::server srv("docroot");

    srv.add_controller(new shop_html_controller());
    srv.add_controller(new shop_rest_controller());

    srv.bind("127.0.0.1", 8080);

    // Note that the rest controller above is not thread safe!
    srv.run(1);

    return 0;
}
//]