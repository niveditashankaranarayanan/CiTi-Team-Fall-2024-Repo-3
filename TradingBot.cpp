#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "include/httplib.h"
#include "include/json.hpp"

using json = nlohmann::json;
using namespace httplib;
using namespace std;

class TradingBot {
public:
    // Function to execute aggressive orders when the time runs out and quantity remains
    map<string, vector<int>> aggressiveOrders(
        const string &sym, const string &action, int qty, vector<string> &bookLevels,
        map<string, map<string, map<string, double>>> &marketDictLocal,
        const string &bookSide, const string &side, double vwapSym,
        const string &strategy, int internalID, int &size) {

        vector<int> orderPrices;
        vector<int> orderQty;
        
        if (!bookLevels.empty()) {
            string level = bookLevels.front();
            bookLevels.erase(bookLevels.begin());

            try {
                int levelSize = marketDictLocal[sym][level][bookSide + "Size"];
                int levelPrice = marketDictLocal[sym][level][bookSide + "Price"];

                cout << "Level: " << level << ", Size in the level: " << levelSize << endl;
                cout << "Price in the level: " << levelPrice << endl;

                int sizeLevel = min(qty - size, levelSize);
                size += sizeLevel;

                orderPrices.push_back(levelPrice);
                orderQty.push_back(sizeLevel);
                
                // Log the order details
                cout << "Creating aggressive order for " << sym << ": OrderNo " << internalID 
                     << ", Side: " << side << ", OrigQty: " << sizeLevel 
                     << ", RemainingQty: " << sizeLevel << ", Price: " << levelPrice << endl;

                // Send the order using a hypothetical send_order method
                // self.send_order(sym, levelPrice, sizeLevel, side, strategy, internalID)
                
            } catch (exception &e) {
                cerr << "Error occurred while executing Aggressive Orders: " << e.what() << endl;
            }
        }

        std::map<std::string, std::vector<int>> response = {{"order_prices", orderPrices}, {"order_qty", orderQty}, {"size", {size}}};

        return response;
    }

    // Function to execute TWAP orders
    std::map<std::string, std::vector<int>> execute_twap_orders(
        const std::string &sym, const std::string &action, int qty, int nSlices,
        std::vector<std::string> &bookLevels,
        std::map<std::string, std::map<std::string, std::map<std::string, double>>> &marketDictLocal,
        const std::string &bookSide, const std::string &side, double vwapSym,
        double &preVwap, int n_slices_iterator, const std::string &strategy,
        int internalID, int size, int targetQ) {

        std::map<std::string, std::vector<int>> response;
        std::vector<int> orderPrices;
        std::vector<int> orderQty;
        
        string level = bookLevels.front();
        bookLevels.erase(bookLevels.begin());

        try {
            int levelSize = marketDictLocal[sym][level][bookSide + "Size"];
            int levelPrice = marketDictLocal[sym][level][bookSide + "Price"];
            int sizeLevel = min(targetQ - size, levelSize);
            size += sizeLevel;

            orderPrices.push_back(levelPrice);
            orderQty.push_back(sizeLevel);

            // Log key metrics
            cout << "TWAP Order: Symbol: " << sym 
                 << ", Level Price: " << levelPrice
                 << ", Size Level: " << sizeLevel
                 << ", VWAP: " << vwapSym 
                 << ", PreVWAP: " << preVwap << endl;
             
            // Send the order using a hypothetical send_order method
            // self.send_order(sym, levelPrice, sizeLevel, side, strategy, internalID)

            // Check conditions for aggressive order completion
            if (n_slices_iterator >= nSlices && qty > size) {
                aggressiveOrders(sym, action, qty, bookLevels, marketDictLocal, bookSide, side, vwapSym, strategy, internalID, size);
            }

        } catch (exception &e) {
            cerr << "Error occurred while executing TWAP Orders: " << e.what() << endl;
        }

        response["order_prices"] = orderPrices;
        response["order_qty"] = orderQty;
        response["size"] = {size};

        return response;
    }
};

void handlePlaceOrders(const Request &req, Response &res) {
    json payload = json::parse(req.body);

    TradingBot bot;

    string sym = payload["sym"];
    string action = payload["action"];
    int qty = payload["qty"];
    int nSlices = payload["n_slices"];
    vector<string> bookLevels = payload["book_levels"];
    map<string, map<string, map<string, double>>> marketDictLocal = payload["market_dict_local"];
    string bookSide = payload["book_side"];
    string side = payload["side"];
    double vwapSym = payload["vwap_sym"];
    double preVwap = payload["pre_vwap"];
    int n_slices_iterator = payload["n_slices_iterator"];
    string strategy = payload["strategy"];
    int internalID = payload["internalID"];
    int size = payload["size"];
    int targetQ = payload["targetQ"];

    map<string, vector<int>> orders = bot.execute_twap_orders(sym, action, qty, nSlices, bookLevels, marketDictLocal, bookSide, side, vwapSym, preVwap, n_slices_iterator, strategy, internalID, size, targetQ);

    res.set_content(json(orders).dump(), "application/json");
}

void handleAggressiveOrders(const Request &req, Response &res) {
    json payload = json::parse(req.body);

    TradingBot bot;

    string sym = payload["sym"];
    string action = payload["action"];
    int qty = payload["qty"];
    vector<string> bookLevels = payload["book_levels"];
    map<string, map<string, map<string, double>>> marketDictLocal = payload["market_dict_local"];
    string bookSide = payload["book_side"];
    string side = payload["side"];
    double vwapSym = payload["vwap_sym"];
    string strategy = payload["strategy"];
    int internalID = payload["internalID"];
    int size = payload["size"];

    map<string, vector<int>> orders = bot.aggressiveOrders(sym, action, qty, bookLevels, marketDictLocal, bookSide, side, vwapSym, strategy, internalID, size);

    res.set_content(json(orders).dump(), "application/json");
}

int main() {
    Server server;

    server.Post("/api/placeOrders", handlePlaceOrders);
    server.Post("/api/placeAggressiveOrders", handleAggressiveOrders);

    server.Get("/api/test", [](const Request &, Response &res) {
        res.set_content("Test endpoint is working", "text/plain");
    });

    server.Get("/api/testPost", [](const Request &req, Response &res) {
        res.set_content("This endpoint only supports POST requests", "text/plain");
    });

    cout << "Server started on port 8000" << endl;
    server.listen("0.0.0.0", 8000);
}