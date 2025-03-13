#include "TwsApi.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <functional>
#include <algorithm>  // for std::find
#include <ctime>  // for time()



#include "EReader.h"        // Include full definition of EReader.
#include "OrderState.h"     // Include OrderState definition.
#include "Decimal.h"

static std::vector<std::string> splitSymbols(const std::string& symbols) {
    std::vector<std::string> result;
    std::istringstream ss(symbols);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Remove leading/trailing whitespace.
        token.erase(token.begin(), std::find_if(token.begin(), token.end(),
                         [](unsigned char ch) { return !std::isspace(ch); }));
        token.erase(std::find_if(token.rbegin(), token.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); }).base(),
                         token.end());
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

static std::string removeSpaces(const std::string& input) {
    std::string output = input;
    output.erase(std::remove(output.begin(), output.end(), ' '), output.end());
    return output;
}

// Constructor: create the EClientSocket instance and initialize the order counter.
TwsApi::TwsApi() : m_client(nullptr), m_signal(nullptr), m_nextOrderId(0) {
    m_signal = new EReaderOSSignal(1000);  // Create a signal with a 1000 ms timeout
    m_client = new EClientSocket(this, m_signal);  // Pass the signal to the client socket
}

TwsApi::~TwsApi() {
    disconnect();
    delete m_client;
}

bool TwsApi::connect(const std::string& host, int port, int clientId) {
    bool connected = m_client->eConnect(host.c_str(), port, clientId);
    if (connected) {
        // Create and start the EReader for asynchronous message processing.
        EReader* reader = new EReader(m_client, m_signal);
        reader->start();

        // Launch a thread that processes incoming messages.
        std::thread readerThread([this, reader]() {
            while (m_client->isConnected()) {
                m_signal->waitForSignal();
                reader->processMsgs();
            }
            delete reader; // Clean up when disconnected.
        });
        readerThread.detach();

        // Wait for the nextValidId callback to set m_nextOrderId.
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait_for(lock, std::chrono::seconds(2));
    }
    return connected;
}



void TwsApi::disconnect() {
    if(m_client)
        m_client->eDisconnect();
}

// --- Order Functions ---

OrderResult TwsApi::submit_order_stock(const std::string& symbol, int qty, const std::string& side,
    const std::string& type, const std::string& time_in_force,
    double limit_price, double stop_price, const std::string& client_order_id,
    double bracket_take_profit_price, double bracket_stop_loss_price, bool is_bracket)
{
    Contract contract = createStockContract(symbol);

    Order parent;
    parent.action = (side == "buy" || side == "BUY") ? "BUY" : "SELL";
    parent.totalQuantity = DecimalFunctions::stringToDecimal(std::to_string(qty));
    parent.orderRef = client_order_id;
    parent.tif = time_in_force;
    parent.transmit = !is_bracket;

    if (type == "market" || type == "MKT") {
        parent.orderType = "MKT";
    } else if (type == "limit" || type == "LMT") {
        parent.orderType = "LMT";
        parent.lmtPrice = limit_price;
    } else if (type == "stop" || type == "STP") {
        parent.orderType = "STP";
        parent.auxPrice = stop_price;
    } else if (type == "stop_limit" || type == "STP LMT") {
        parent.orderType = "STP LMT";
        parent.lmtPrice = limit_price;
        parent.auxPrice = stop_price;
    }

    OrderId parentOrderId, tpOrderId, slOrderId;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        parentOrderId = m_nextOrderId++;
    }
    parent.orderId = parentOrderId;

    m_client->placeOrder(parentOrderId, contract, parent);

    if (is_bracket) {
        Order takeProfit;
        takeProfit.action = (side == "buy" || side == "BUY") ? "SELL" : "BUY";
        takeProfit.orderType = "LMT";
        takeProfit.totalQuantity = parent.totalQuantity;
        takeProfit.lmtPrice = bracket_take_profit_price;
        takeProfit.parentId = parentOrderId;
        takeProfit.transmit = false;

        Order stopLoss;
        stopLoss.action = takeProfit.action;
        stopLoss.orderType = "STP";
        stopLoss.auxPrice = bracket_stop_loss_price;
        stopLoss.totalQuantity = parent.totalQuantity;
        stopLoss.parentId = parentOrderId;
        stopLoss.transmit = true;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            tpOrderId = m_nextOrderId++;
            slOrderId = m_nextOrderId++;
        }

        m_client->placeOrder(tpOrderId, contract, takeProfit);
        m_client->placeOrder(slOrderId, contract, stopLoss);

        parent.transmit = false;
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        parentOrderId = m_nextOrderId++;
    }
    parent.orderId = parentOrderId;
    parent.orderRef = client_order_id;
    parent.transmit = !is_bracket;

    OrderResult result;
    result.orderId = parent.orderId;
    result.status = is_bracket ? "Bracket Pending" : "Pending";

    return result;
}


OrderResult TwsApi::submit_order_option(const std::string& symbol, int qty, const std::string& side,
    const std::string& type, const std::string& time_in_force,
    double limit_price, double stop_price, const std::string& client_order_id,
    double bracket_take_profit_price, double bracket_stop_loss_price, bool is_bracket)
{

    Contract contract = createOptionContract(symbol);

    Order parent;
    parent.action = (side == "buy" || side == "BUY") ? "BUY" : "SELL";
    parent.totalQuantity = DecimalFunctions::stringToDecimal(std::to_string(qty));
    parent.orderRef = client_order_id;
    parent.tif = time_in_force;
    parent.transmit = !is_bracket;

    if (type == "market" || type == "MKT") {
        parent.orderType = "MKT";
    } else if (type == "limit" || type == "LMT") {
        parent.orderType = "LMT";
        parent.lmtPrice = limit_price;
    } else if (type == "stop" || type == "STP") {
        parent.orderType = "STP";
        parent.auxPrice = stop_price;
    } else if (type == "stop_limit" || type == "STP LMT") {
        parent.orderType = "STP LMT";
        parent.lmtPrice = limit_price;
        parent.auxPrice = stop_price;
    }

    OrderId parentOrderId, tpOrderId, slOrderId;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        parentOrderId = m_nextOrderId++;
    }

    m_client->placeOrder(parentOrderId, contract, parent);

    if (is_bracket) {
        Order takeProfit;
        takeProfit.action = (side == "buy" || side == "BUY") ? "SELL" : "BUY";
        takeProfit.orderType = "LMT";
        takeProfit.totalQuantity = parent.totalQuantity;
        takeProfit.lmtPrice = bracket_take_profit_price;
        takeProfit.parentId = parentOrderId;
        takeProfit.transmit = false;

        Order stopLoss;
        stopLoss.action = takeProfit.action;
        stopLoss.orderType = "STP";
        stopLoss.auxPrice = bracket_stop_loss_price;
        stopLoss.totalQuantity = parent.totalQuantity;
        stopLoss.parentId = parentOrderId;
        stopLoss.transmit = true;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            tpOrderId = m_nextOrderId++;
            slOrderId = m_nextOrderId++;
        }

        m_client->placeOrder(tpOrderId, contract, takeProfit);
        m_client->placeOrder(slOrderId, contract, stopLoss);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::unique_lock<std::mutex> lock(m_mutex);

    OrderResult result;
    if(m_orders.find(parentOrderId) != m_orders.end())
        result = m_orders[parent.orderId];
    else {
        result.orderId = parent.orderId;
        result.status = is_bracket ? "Bracket Pending" : "Pending";
    }

    return result;
}


std::vector<OrderResult> TwsApi::list_orders(const std::string& /*status*/, int limit,
    const std::string& /*after*/, const std::string& /*until*/,
    const std::string& direction, const std::string& symbols,
    const std::string& side)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    std::vector<OrderResult> filteredOrders;

    bool filterSymbols = !symbols.empty();
    bool filterSide    = !side.empty();

    std::set<std::string> symbolSet;
    if (filterSymbols) {
        std::istringstream symStream(symbols);
        std::string token;
        while (std::getline(symStream, token, ',')) {
            // Trim leading whitespace.
            token.erase(token.begin(),
                std::find_if(token.begin(), token.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            // Trim trailing whitespace.
            token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                        token.end());
            if (!token.empty())
                symbolSet.insert(token);
        }
    }

    // Iterate over all stored orders and apply filters.
    for (const auto& pair : m_orders) {
        const OrderResult& order = pair.second;

        if (filterSymbols && symbolSet.find(order.symbol) == symbolSet.end())
            continue;

        if (filterSide && order.side != side)
            continue;

        filteredOrders.push_back(order);
    }

    if (direction == "desc") {
        std::sort(filteredOrders.begin(), filteredOrders.end(), [](const OrderResult& a, const OrderResult& b) {
            return a.timestamp > b.timestamp;
        });
    } else { // Default to ascending order.
        std::sort(filteredOrders.begin(), filteredOrders.end(), [](const OrderResult& a, const OrderResult& b) {
            return a.timestamp < b.timestamp;
        });
    }

    // Limit the number of orders returned.
    if (limit > 0 && filteredOrders.size() > static_cast<size_t>(limit))
        filteredOrders.resize(limit);

    return filteredOrders;
}

void TwsApi::reqAllOpenOrders()
{
    if (m_client) {
        m_orders.clear();
        m_client->reqAllOpenOrders();
    } else {
        std::cerr << "error at reqAllOpenOrders" << std::endl;
    }
}
// Callback invoked when TWS signals the end of open orders.
void TwsApi::openOrderEnd(){}

void TwsApi::cancel_order(OrderId order_id) {
    OrderCancel orderCancel;
    m_client->cancelOrder(order_id, orderCancel);
    std::unique_lock<std::mutex> lock(m_mutex);
}

// --- Position Functions ---

std::vector<Position> TwsApi::list_positions() {
    m_positions.clear();
    m_client->reqPositions();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_positions;
}

Position TwsApi::get_position(const std::string& symbol) {
    auto positions = list_positions();
    for(const auto& pos : positions) {
        if(pos.symbol == symbol)
            return pos;
    }
    std::cerr << "error at get_position: " << symbol << " not found" <<  std::endl;
}

// --- Order Modification and Query ---

OrderResult TwsApi::change_order_by_order_id(OrderId order_id,
    int qty, std::string time_in_force,
    std::optional<double> limit_price, std::optional<double> stop_price)
{
    // Lock and lookup the original order.
    std::unique_lock<std::mutex> lock(m_mutex);
    auto it = m_orders.find(order_id);
    if (it == m_orders.end()) {
        OrderResult res;
        res.orderId = order_id;
        res.status = "OrderNotFound";
        return res;
    }

    OrderResult orig = it->second;

    // Make sure parent/child relationships are respected when modifying bracket orders.
    Order parentOrder;
    parentOrder.orderId = order_id;
    parentOrder.action = orig.side;
    parentOrder.totalQuantity = DecimalFunctions::stringToDecimal(std::to_string((qty != 0) ? qty : orig.qty));
    parentOrder.tif = (time_in_force != "-") ? time_in_force : orig.tif;

    // Update only specified fields, retain original if not provided.
    parentOrder.lmtPrice = limit_price.value_or(orig.limit_price);
    parentOrder.auxPrice = stop_price.value_or(orig.stop_price);

    if (parentOrder.lmtPrice != 0.0 && parentOrder.auxPrice != 0.0)
        parentOrder.orderType = "STP LMT";
    else if (parentOrder.lmtPrice != 0.0)
        parentOrder.orderType = "LMT";
    else if (parentOrder.auxPrice != 0.0)
        parentOrder.orderType = "STP";
    else
        parentOrder.orderType = orig.orderType;

    parentOrder.tif = parentOrder.tif.empty() ? orig.tif : time_in_force;
    parentOrder.orderRef = orig.orderRef;

    // Recreate the appropriate contract based on the original order's asset type.
    Contract contract;
    if(orig.assetType == "OPT")
        contract = createOptionContract(orig.symbol);
    else
        contract = createStockContract(orig.symbol);

    // Explicitly set the transmit flag.
    parentOrder.transmit = true;

    // Modify the order in TWS.
    m_client->placeOrder(order_id, contract, parentOrder);

    // Update local record to reflect modification.
    orig.qty = parentOrder.totalQuantity;
    orig.tif = parentOrder.tif;
    orig.limit_price = parentOrder.lmtPrice;
    orig.stop_price = parentOrder.auxPrice;
    orig.orderType = parentOrder.orderType;
    orig.timestamp = std::chrono::system_clock::now();
    orig.status = "Modified";

    m_orders[order_id] = orig;

    return orig;
}

// --- Historical Data ---

std::vector<HistoricalBar> TwsApi::get_historical_data_stocks(const std::string& symbol,
    const std::string& start, const std::string& end, int limit)
{
    Contract contract = createStockContract(symbol);
    int reqId = std::hash<std::string>{}(symbol) % 10000;
    // For a real implementation, properly format the endDateTime and calculate durationStr.
    std::string endDateTime = end;  // This should be in IB’s expected format.
    std::string durationStr = "1 D";  // Dummy duration
    std::string barSizeSetting = "1 day";
    std::string whatToShow = "TRADES";
    int useRTH = 1;
    int formatDate = 1;
    m_client->reqHistoricalData(reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, false, TagValueListSPtr());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<HistoricalBar> bars;
    if(m_historicalData.find(reqId) != m_historicalData.end()) {
        bars = m_historicalData[reqId];
        if(bars.size() > (size_t)limit)
            bars.resize(limit);
    }

    return bars;
}

// Convenience function to get latest trades for one or more symbols.
void TwsApi::subscribe_stock_trades(const std::string& symbols) {
    std::vector<std::string> symbolList = splitSymbols(symbols);
    std::vector<int> tickerIds;

    // Subscribe to tick-by-tick trade data ("Last") for each symbol.
    for (const auto& sym : symbolList) {
        Contract contract = createStockContract(sym);
        int tickerId = m_nextTickerId++;
        m_tickerIdToSymbol[tickerId] = sym;
        // "Last" returns individual trade ticks.
        m_client-> reqTickByTickData(tickerId, contract, "Last", 0, false);
        tickerIds.push_back(tickerId);
    }
}

// Convenience function to get latest quotes for one or more symbols.
void TwsApi::subscribe_stock_quotes(const std::string& symbols) {
    std::vector<std::string> symbolList = splitSymbols(symbols);
    std::vector<int> tickerIds;

    // Subscribe to tick-by-tick quote data ("BidAsk") for each symbol.
    for (const auto& sym : symbolList) {
        Contract contract = createStockContract(sym);
        int tickerId = m_nextTickerId++;
        m_tickerIdToSymbol[tickerId] = sym;
        // "BidAsk" returns bid and ask updates.
        m_client-> reqTickByTickData(tickerId, contract, "BidAsk", 0, false);
        tickerIds.push_back(tickerId);
    }
}

// Convenience function to get latest trades for one or more option symbols.
void TwsApi::subscribe_option_trades(const std::string& symbols) {
    std::vector<std::string> symbolList = splitSymbols(symbols);
    std::vector<int> tickerIds;

    // Subscribe to tick-by-tick trade data ("Last") for each option symbol.
    for (const auto& sym : symbolList) {
        Contract contract = createOptionContract(sym);
        int tickerId = m_nextTickerId++;
        m_tickerIdToSymbol[tickerId] = sym;
        // "Last" returns individual trade ticks.
        m_client-> reqTickByTickData(tickerId, contract, "Last", 0, false);
        tickerIds.push_back(tickerId);
    }
}

// Convenience function to get latest quotes for one or more option symbols.
void TwsApi::subscribe_option_quotes(const std::string& symbols) {
    std::vector<std::string> symbolList = splitSymbols(symbols);
    std::vector<int> tickerIds;

    // Subscribe to tick-by-tick quote data ("BidAsk") for each option symbol.
    for (const auto& sym : symbolList) {
        Contract contract = createOptionContract(sym);
        int tickerId = m_nextTickerId++;
        m_tickerIdToSymbol[tickerId] = sym;
        // "BidAsk" returns bid and ask updates.
        m_client-> reqTickByTickData(tickerId, contract, "BidAsk", 0, false);
        tickerIds.push_back(tickerId);
    }
}

Contract TwsApi::createStockContract(const std::string& symbol) {
    Contract contract;
    contract.symbol = symbol;
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";
    return contract;
}

Contract TwsApi::createOptionContract(const std::string& symbol) {
    size_t pos = 0;
    while (pos < symbol.size() && std::isalpha(symbol[pos])) {
        ++pos;
    }
    if (pos == 0 || symbol.size() < pos + 6 + 1 + 8) {
        throw std::invalid_argument("Symbol format is invalid");
    }

    std::string ticker = symbol.substr(0, pos);
    std::string dateStr = symbol.substr(pos, 6);
    char rightChar = symbol[pos + 6];
    std::string strikeStr = symbol.substr(pos + 6 + 1, 8);

    std::string right;
    if (rightChar == 'C') {
        right = "CALL";
    } else if (rightChar == 'P') {
        right = "PUT";
    } else {
        throw std::invalid_argument("Symbol format is invalid: invalid option type");
    }

    double strike = std::stod(strikeStr) / 1000.0;

    Contract contract;
    contract.symbol = ticker;
    contract.lastTradeDateOrContractMonth = "20" + dateStr;
    contract.strike = strike;
    contract.right = right;
    contract.secType = "OPT";
    contract.exchange = "SMART";
    contract.currency = "USD";
    contract.multiplier = "100";

    return contract;
}


void TwsApi::nextValidId(OrderId orderId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_nextOrderId = orderId;
    m_cond.notify_all();
}

// Callback for tick-by-tick trade ticks ("Last").
void TwsApi::tickByTickAllLast(int reqId, int tickType, time_t time, double price,
                               Decimal size, const TickAttribLast& tickAttribLast,
                               const std::string& exchange, const std::string& specialConditions) {
    Trade trade;
    {
        std::lock_guard<std::mutex> lock(m_tickMutex);
        auto it = m_tickerIdToSymbol.find(reqId);
        trade.symbol = (it != m_tickerIdToSymbol.end()) ? it->second : "UNKNOWN";
        trade.trade_price = price;
        trade.timestamp = time;  // Assuming time_t is already in seconds.
        trade.size = size;
        trade.tickType = tickType;
        // You might want to do something with tickType, size, and tickAttribLast if needed.
        m_latestTrades.push_back(trade);
    }
}

// Callback for tick-by-tick bid/ask ticks ("BidAsk").
void TwsApi::tickByTickBidAsk(int reqId, long time, double bidPrice, double askPrice,
                              int bidSize, int askSize,
                              const std::string& tickAttribBidAsk) {
    Quote quote;
    {
        std::lock_guard<std::mutex> lock(m_tickMutex);
        auto it = m_tickerIdToSymbol.find(reqId);
        quote.symbol = (it != m_tickerIdToSymbol.end()) ? it->second : "UNKNOWN";
        quote.bid_price = bidPrice;
        quote.ask_price = askPrice;
        quote.timestamp = time;
        quote.askSize = askSize;
        quote.bidSize = bidSize;
        m_latestQuotes.push_back(quote);
    }
}

// Función auxiliar para separar un string con delimitador (coma) en un vector de strings.
std::vector<std::string> splitSymbolsFilter(const std::string& symbolsStr) {
    std::vector<std::string> result;
    std::istringstream ss(symbolsStr);
    std::string symbol;
    while (std::getline(ss, symbol, ',')) {
        // Se pueden aplicar transformaciones adicionales (p.ej. trim) si es necesario.
        result.push_back(symbol);
    }
    return result;
}

// Filtra trades de los últimos 'seconds' segundos para los símbolos indicados en el string.
std::vector<Trade> TwsApi::filterTradesForLastSeconds(const std::string& symbols, int seconds) {
    std::vector<std::string> symbolList = splitSymbolsFilter(symbols);
    std::vector<Trade> result;
    long now = static_cast<long>(time(nullptr));
    long threshold = now - seconds;

    for (const auto& trade : m_latestTrades) {
        if (std::find(symbolList.begin(), symbolList.end(), trade.symbol) != symbolList.end() &&
            trade.timestamp >= threshold) {
            result.push_back(trade);
            }
    }
    return result;
}

// Filtra cotizaciones de los últimos 'seconds' segundos para los símbolos indicados en el string.
std::vector<Quote> TwsApi::filterQuotesForLastSeconds(const std::string& symbols, int seconds) {
    std::vector<std::string> symbolList = splitSymbolsFilter(symbols);
    std::vector<Quote> result;
    long now = static_cast<long>(time(nullptr));
    long threshold = now - seconds;

    for (const auto& quote : m_latestQuotes) {
        if (std::find(symbolList.begin(), symbolList.end(), quote.symbol) != symbolList.end() &&
            quote.timestamp >= threshold) {
            result.push_back(quote);
            }
    }
    return result;
}
// Example implementation of cancelTickByTickData (you need to call the underlying client).
void TwsApi::cancelTickByTickData(int tickerId) {
    if (m_client) {
        m_client->cancelTickByTickData(tickerId);
    }
}


void TwsApi::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) {
    std::lock_guard<std::mutex> lock(m_tickMutex);
    auto symbolIt = m_tickerIdToSymbol.find(tickerId);
    if (symbolIt != m_tickerIdToSymbol.end()) {
        std::string symbol = symbolIt->second;
        Quote& quote = m_quotes[tickerId];
        quote.symbol = symbol;
        quote.timestamp = std::time(nullptr);

        if (field == BID)
            quote.bid_price = price;
        else if (field == ASK)
            quote.ask_price = price;
        else if (field == LAST)
            quote.last_price = price;
        else if (field == CLOSE)
            quote.close_price = price;
    }
}

void TwsApi::orderStatus(OrderId orderId, const std::string& status, Decimal /*filled*/,
    Decimal /*remaining*/, double /*avgFillPrice*/, long long /*permId*/, int /*parentId*/,
    double /*lastFillPrice*/, int /*clientId*/, const std::string& /*whyHeld*/, double /*mktCapPrice*/) {
    // std::unique_lock<std::mutex> lock(m_mutex);
    // std::string client_order_id = std::to_string(orderId);
    // OrderResult result;
    // result.status = status;
    // result.orderId = orderId;
    // m_orders[orderId] = result;
    // std::cout << "dentro de orderStatus" << std::endl;
    // m_cond.notify_all();
}


void TwsApi::openOrder(OrderId orderId, const Contract& contract, const Order& order, const OrderState& orderState) {
    std::unique_lock<std::mutex> lock(m_mutex);
    OrderResult result;
    result.orderId = orderId;
    result.assetType = contract.secType;
    result.orderRef = order.orderRef;
    result.status = orderState.status;
    if (contract.secType == "OPT") {
        result.symbol = removeSpaces(contract.localSymbol);
    }
    else result.symbol = contract.symbol;
    result.side = order.action;
    result.qty = order.totalQuantity;
    result.orderType = order.orderType;
    result.limit_price = order.lmtPrice;
    result.stop_price = order.auxPrice;
    result.tif = order.tif;
    m_orders[order.orderId] = result;
}

void TwsApi::historicalData(TickerId reqId, const Bar& bar) {
    std::unique_lock<std::mutex> lock(m_mutex);
    HistoricalBar hbar;
    hbar.time = bar.time;
    hbar.open = bar.open;
    hbar.high = bar.high;
    hbar.low = bar.low;
    hbar.close = bar.close;
    hbar.volume = bar.volume;
    m_historicalData[reqId].push_back(hbar);
}

void TwsApi::historicalDataEnd(int /*reqId*/, const std::string& /*startDateStr*/, const std::string& /*endDateStr*/) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.notify_all();
}


void TwsApi::requestMarketData(const std::string& symbol) {
    Contract contract = createStockContract(symbol);
    int tickerId = m_nextTickerId++;
    m_tickerIdToSymbol[tickerId] = symbol;
    m_client->reqMktData(tickerId, contract, "", false, false, TagValueListSPtr());
}

void TwsApi::cancelMarketData(int tickerId) {
    m_client->cancelMktData(tickerId);
}


// --- Other callbacks are left with empty implementations or basic logging ---

void TwsApi::tickSize(TickerId, TickType, Decimal) { }
void TwsApi::tickOptionComputation(TickerId, TickType, int, double, double, double, double, double, double, double, double) { }
void TwsApi::tickGeneric(TickerId, TickType, double) { }
void TwsApi::tickString(TickerId, TickType, const std::string&) { }
void TwsApi::tickEFP(TickerId, TickType, double, const std::string&, double, int, const std::string&, double, double) { }
void TwsApi::winError(const std::string&, int) { }
void TwsApi::connectionClosed() { }
void TwsApi::updateAccountValue(const std::string&, const std::string&, const std::string&, const std::string&) { }
void TwsApi::updatePortfolio(const Contract&, Decimal, double, double, double, double, double, const std::string&) { }
void TwsApi::updateAccountTime(const std::string&) { }
void TwsApi::accountDownloadEnd(const std::string&) { }
void TwsApi::contractDetails(int, const ContractDetails&) { }
void TwsApi::bondContractDetails(int, const ContractDetails&) { }
void TwsApi::contractDetailsEnd(int) { }
void TwsApi::execDetails(int, const Contract&, const Execution&) { }
void TwsApi::execDetailsEnd(int) { }
void TwsApi::error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) {
    // std::unique_lock<std::mutex> lock(m_mutex);
    // // ANSI escape code for green text: "\033[32m"
    // // Reset code: "\033[0m"
    // std::cerr << "\033[32mError (id " << id << ") at " << std::ctime(&errorTime)
    //           << "Code: " << errorCode << " - " << errorString << "\033[0m" << std::endl;
    // m_cond.notify_all();
}
void TwsApi::updateMktDepth(TickerId, int, int, int, double, Decimal) { }
void TwsApi::updateMktDepthL2(TickerId, int, const std::string&, int, int, double, Decimal, bool) { }
void TwsApi::updateNewsBulletin(int, int, const std::string&, const std::string&) { }
void TwsApi::managedAccounts(const std::string&) { }
void TwsApi::receiveFA(faDataType, const std::string&) { }
void TwsApi::scannerParameters(const std::string&) { }
void TwsApi::scannerData(int, int, const ContractDetails&, const std::string&, const std::string&, const std::string&, const std::string&) { }
void TwsApi::scannerDataEnd(int) { }
void TwsApi::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                           Decimal volume, Decimal wap, int count) { }
void TwsApi::currentTime(long) { }
void TwsApi::fundamentalData(TickerId, const std::string&) { }
void TwsApi::deltaNeutralValidation(int, const DeltaNeutralContract&) { }
void TwsApi::tickSnapshotEnd(int) { }
void TwsApi::marketDataType(TickerId, int) { }
void TwsApi::commissionAndFeesReport(const CommissionAndFeesReport&) { }
void TwsApi::position(const std::string& /*account*/, const Contract& contract, Decimal position, double avgCost) {
    Position pos;
    if (contract.secType == "STK") pos.symbol = contract.symbol;
    else pos.symbol = removeSpaces(contract.localSymbol);
    pos.qty = static_cast<int>(position);
    pos.avgCost = avgCost;
    m_positions.push_back(pos);
}
void TwsApi::positionEnd() { }
void TwsApi::accountSummary(int, const std::string&, const std::string&, const std::string&, const std::string&) { }
void TwsApi::accountSummaryEnd(int) { }
void TwsApi::verifyMessageAPI(const std::string&) { }
void TwsApi::verifyCompleted(bool, const std::string&) { }
void TwsApi::displayGroupList(int, const std::string&) { }
void TwsApi::displayGroupUpdated(int, const std::string&) { }
void TwsApi::verifyAndAuthMessageAPI(const std::string&, const std::string&) { }
void TwsApi::verifyAndAuthCompleted(bool, const std::string&) { }
void TwsApi::connectAck() { }
void TwsApi::positionMulti(int reqId, const std::string& account, const std::string& modelCode, const Contract& contract, Decimal pos, double avgCost) { }
void TwsApi::positionMultiEnd(int) { }
void TwsApi::accountUpdateMulti(int, const std::string&, const std::string&, const std::string&, const std::string&, const std::string&) { }
void TwsApi::accountUpdateMultiEnd(int) { }
void TwsApi::securityDefinitionOptionalParameter(int, const std::string&, int, const std::string&, const std::string&, const std::set<std::string>&, const std::set<double>&) { }
void TwsApi::securityDefinitionOptionalParameterEnd(int) { }
void TwsApi::softDollarTiers(int, const std::vector<SoftDollarTier>&) { }
void TwsApi::familyCodes(const std::vector<FamilyCode>&) { }
void TwsApi::symbolSamples(int, const std::vector<ContractDescription>&) { }
void TwsApi::mktDepthExchanges(const std::vector<DepthMktDataDescription>&) { }
void TwsApi::tickNews(int, time_t, const std::string&, const std::string&, const std::string&, const std::string&) { }
void TwsApi::smartComponents(int, const SmartComponentsMap&) { }
void TwsApi::tickReqParams(int, double, const std::string&, int) { }
void TwsApi::newsProviders(const std::vector<NewsProvider>&) { }
void TwsApi::newsArticle(int, int, const std::string&) { }
void TwsApi::historicalNews(int, const std::string&, const std::string&, const std::string&, const std::string&) { }
void TwsApi::historicalNewsEnd(int, bool) { }
void TwsApi::headTimestamp(int, const std::string&) { }
void TwsApi::histogramData(int, const HistogramDataVector&) { }
void TwsApi::historicalDataUpdate(TickerId, const Bar&) { }
void TwsApi::rerouteMktDataReq(int, int, const std::string&) { }
void TwsApi::rerouteMktDepthReq(int, int, const std::string&) { }
void TwsApi::marketRule(int, const std::vector<PriceIncrement>&) { }
void TwsApi::pnl(int, double, double, double) { }
void TwsApi::pnlSingle(int, Decimal, double, double, double, double) { }
void TwsApi::historicalTicks(int, const std::vector<HistoricalTick>&, bool) { }
void TwsApi::historicalTicksBidAsk(int, const std::vector<HistoricalTickBidAsk>&, bool) { }
void TwsApi::historicalTicksLast(int, const std::vector<HistoricalTickLast>&, bool) { }
void TwsApi::tickByTickBidAsk(int, time_t, double, double, Decimal, Decimal, const TickAttribBidAsk&) { }
void TwsApi::tickByTickMidPoint(int, time_t, double) { }
void TwsApi::orderBound(long long, int, int) { }
void TwsApi::completedOrder(const Contract&, const Order&, const OrderState&) { }
void TwsApi::completedOrdersEnd() { }
void TwsApi::replaceFAEnd(int, const std::string&) { }
void TwsApi::wshMetaData(int, const std::string&) { }
void TwsApi::wshEventData(int, const std::string&) { }
void TwsApi::historicalSchedule(int, const std::string&, const std::string&, const std::string&, const std::vector<HistoricalSession>&) { }
void TwsApi::userInfo(int, const std::string&) { }
void TwsApi::currentTimeInMillis(time_t) { }
    // Empty implementation for now.

