#include "TwsApi.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <functional>
#include "EReader.h"        // Include full definition of EReader.
#include "OrderState.h"     // Include OrderState definition.
#include "Decimal.h"

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
    double limit_price, double stop_price, const std::string& client_order_id)
{
    Contract contract = createStockContract(symbol);
    Order order;
    order.action = (side == "buy" || side == "BUY") ? "BUY" : "SELL";

    if (type == "market" || type == "MKT") {
        order.orderType = "MKT";
        // For market orders, explicitly clear any price fields.
        order.lmtPrice = 0.0;
        order.auxPrice = 0.0;
    } else if (type == "limit" || type == "LMT") {
        order.orderType = "LMT";
        order.lmtPrice = limit_price;
    } else if (type == "stop" || type == "STP") {
        order.orderType = "STP";
        order.auxPrice = stop_price;
    } else if (type == "stop_limit" || type == "STP LMT") {
        order.orderType = "STP LMT";
        order.lmtPrice = limit_price;
        order.auxPrice = stop_price;
    }

    // Convert qty to a Decimal using the helper function.
    order.totalQuantity = DecimalFunctions::stringToDecimal(std::to_string(qty));
    order.tif = time_in_force;  // e.g. "DAY" or "GTC"
    order.orderRef = client_order_id;

    std::cout << "Placing order:"
              << "\n  symbol = " << symbol
              << "\n  qty = " << qty
              << "\n  orderType = " << order.orderType
              << "\n  totalQuantity = " << std::to_string(qty)
              << "\n  tif = " << order.tif << std::endl;

    int orderId;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        orderId = m_nextOrderId++;
    }
    m_client->placeOrder(orderId, contract, order);

    // Wait until order status is updated.
    std::unique_lock<std::mutex> lock(m_mutex);
    bool updated = m_cond.wait_for(lock, std::chrono::milliseconds(500),
                        [&]{ return m_orders.find(client_order_id) != m_orders.end(); });
    OrderResult result;
    if (updated)
        result = m_orders[client_order_id];
    else {
        result.id = client_order_id;
        result.status = "Pending";
    }
    return result;
}

OrderResult TwsApi::submit_order_option(const std::string& symbol, int qty, const std::string& side,
    const std::string& type, const std::string& time_in_force,
    double limit_price, double stop_price, const std::string& client_order_id,
    double bracket_take_profit_price, double bracket_stop_loss_price)
{
    Contract contract = createOptionContract(symbol);
    Order order;
    order.action = (side == "buy" || side == "BUY") ? "BUY" : "SELL";

    if(type == "market" || type == "MKT")
        order.orderType = "MKT";
    else if(type == "limit" || type == "LMT") {
        order.orderType = "LMT";
        order.lmtPrice = limit_price;
    } else if(type == "stop" || type == "STP") {
        order.orderType = "STP";
        order.auxPrice = stop_price;
    } else if(type == "stop_limit" || type == "STP LMT") {
        order.orderType = "STP LMT";
        order.lmtPrice = limit_price;
        order.auxPrice = stop_price;
    }

    order.totalQuantity = qty;
    order.tif = time_in_force;
    order.orderRef = client_order_id;
    // For bracket orders, IB requires multiple legs. This demo does not implement full bracket logic.

    int orderId;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        orderId = m_nextOrderId++;
    }
    m_client->placeOrder(orderId, contract, order);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::unique_lock<std::mutex> lock(m_mutex);
    OrderResult result;
    if(m_orders.find(client_order_id) != m_orders.end())
        result = m_orders[client_order_id];
    else {
        result.id = client_order_id;
        result.status = "Pending";
    }
    return result;
}

std::vector<OrderResult> TwsApi::list_orders(const std::string& /*status*/, int limit,
    const std::string& /*after*/, const std::string& /*until*/,
    const std::string& /*direction*/, const std::string& /*symbols*/,
    const std::string& /*side*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<OrderResult> orders;
    for(auto& pair : m_orders) {
        orders.push_back(pair.second);
        if(orders.size() >= (size_t)limit)
            break;
    }
    return orders;
}

void TwsApi::cancel_order(const std::string& order_id) {
    int ibOrderId = std::stoi(order_id);
    OrderCancel orderCancel; // Create a default cancellation request
    m_client->cancelOrder(ibOrderId, orderCancel);
    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_orders.find(order_id) != m_orders.end())
        m_orders[order_id].status = "Cancelled";
}

// --- Position Functions ---

std::vector<Position> TwsApi::list_positions() {
    m_client->reqPositions();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_positions;
}

Position TwsApi::get_position(const std::string& symbol) {
    auto positions = list_positions();
    for(const auto& pos : positions) {
        if(pos.symbol == symbol)
            return pos;
    }
    return Position{"", 0, 0.0};
}

// --- Market Data Functions ---

std::vector<Quote> TwsApi::get_latest_quotes_stocks(const std::string& symbols, const std::string& /*currency*/) {
    std::vector<Quote> quotes;
    std::istringstream ss(symbols);
    std::string token;
    while(std::getline(ss, token, ',')) {
        Contract contract = createStockContract(token);
        int tickerId = std::hash<std::string>{}(token) % 10000;
        m_client->reqMktData(tickerId, contract, "", false, false, nullptr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_quotes.find(tickerId) != m_quotes.end())
            quotes.push_back(m_quotes[tickerId]);
    }
    return quotes;
}

std::vector<Quote> TwsApi::get_latest_quotes_options(const std::string& symbols) {
    std::vector<Quote> quotes;
    std::istringstream ss(symbols);
    std::string token;
    while(std::getline(ss, token, ',')) {
        Contract contract = createOptionContract(token);
        int tickerId = std::hash<std::string>{}(token) % 10000;
        m_client->reqMktData(tickerId, contract, "", false, false, nullptr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_quotes.find(tickerId) != m_quotes.end())
            quotes.push_back(m_quotes[tickerId]);
    }
    return quotes;
}

std::vector<Trade> TwsApi::get_latest_trades_stocks(const std::string& symbols, const std::string& /*currency*/) {
    std::vector<Trade> trades;
    std::istringstream ss(symbols);
    std::string token;
    while (std::getline(ss, token, ',')) {
        Contract contract = createStockContract(token);
        // Generate a unique tickerId for this symbol.
        int tickerId = (std::hash<std::string>{}(token) % 10000) + 10000;

        // Store the mapping from tickerId to symbol.
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_reqIdToSymbol[tickerId] = token;
        }

        // Request tick-by-tick data for the last trade.
        m_client->reqTickByTickData(tickerId, contract, "Last", 0, false);

        // Wait briefly until the tick callback updates m_trades.
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // Wait up to 100 milliseconds or until trade data appears.
            m_cond.wait_for(lock, std::chrono::milliseconds(100),
                              [&]{ return m_trades.find(tickerId) != m_trades.end(); });
        }

        // Retrieve the trade if available.
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_trades.find(tickerId) != m_trades.end()) {
                Trade trade = m_trades[tickerId];
                // If trade.symbol is still empty, fill it from the mapping.
                if (trade.symbol.empty())
                    trade.symbol = m_reqIdToSymbol[tickerId];
                trades.push_back(trade);
            } else {
                std::cerr << "No trade data received for symbol " << token
                          << " (tickerId: " << tickerId << ")" << std::endl;
            }
        }
    }
    return trades;
}

void TwsApi::tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
                               const TickAttribLast& tickAttribLast, const std::string& exchange,
                               const std::string& specialConditions) {
    std::unique_lock<std::mutex> lock(m_mutex);
    Trade trade;
    auto it = m_reqIdToSymbol.find(reqId);
    trade.symbol = (it != m_reqIdToSymbol.end()) ? it->second : "Unknown";
    trade.trade_price = price;
    trade.timestamp = time;
    m_trades[reqId] = trade;
    m_cond.notify_all();
}

std::vector<Trade> TwsApi::get_latest_trades_options(const std::string& symbols) {
    std::vector<Trade> trades;
    std::istringstream ss(symbols);
    std::string token;
    while(std::getline(ss, token, ',')) {
        Contract contract = createOptionContract(token);
        int tickerId = (std::hash<std::string>{}(token) % 10000) + 10000;
        m_client->reqMktData(tickerId, contract, "", false, false, nullptr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_trades.find(tickerId) != m_trades.end())
            trades.push_back(m_trades[tickerId]);
    }
    return trades;
}

// --- Order Modification and Query ---

OrderResult TwsApi::change_order_by_order_id(const std::string& client_order_id,
    std::optional<int> qty, std::optional<std::string> time_in_force,
    std::optional<double> limit_price, std::optional<double> stop_price)
{
    OrderResult existing = get_order(client_order_id);
    cancel_order(client_order_id);
    // In a full implementation you would save all order parameters.
    // Here we simply re-submit a new (dummy) order.
    std::string symbol = "UNKNOWN";  // Replace with the stored symbol
    int newQty = qty.value_or(0);
    std::string newTif = time_in_force.value_or("DAY");
    double newLimit = limit_price.value_or(0.0);
    double newStop = stop_price.value_or(0.0);
    OrderResult modified = submit_order_stock(symbol, newQty, "buy", "limit", newTif, newLimit, newStop, client_order_id);
    return modified;
}

OrderResult TwsApi::get_order(const std::string& order_id) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_orders.find(order_id) != m_orders.end())
        return m_orders[order_id];
    return OrderResult{order_id, "NotFound"};
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
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<HistoricalBar> bars;
    if(m_historicalData.find(reqId) != m_historicalData.end()) {
        bars = m_historicalData[reqId];
        if(bars.size() > (size_t)limit)
            bars.resize(limit);
    }
    return bars;
}

// --- Helper Functions to Create Contracts ---

Contract TwsApi::createStockContract(const std::string& symbol) {
    Contract contract;
    contract.symbol = symbol;
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";
    return contract;
}

Contract TwsApi::createOptionContract(const std::string& symbol) {
    Contract contract;
    contract.symbol = symbol;
    contract.secType = "OPT";
    contract.exchange = "SMART";
    contract.currency = "USD";
    // In a real implementation, you would specify the expiry, strike, right, etc.
    contract.lastTradeDateOrContractMonth = "20241220";
    contract.strike = 100.0;
    contract.right = "C";
    contract.multiplier = "100";
    return contract;
}

// --- EWrapper Callback Implementations ---

void TwsApi::nextValidId(OrderId orderId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_nextOrderId = orderId;
    m_cond.notify_all();
}

void TwsApi::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& /*attrib*/) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if(field == LAST) {
        Trade trade;
        trade.symbol = "";  // In a complete implementation, map tickerId to symbol.
        trade.trade_price = price;
        m_trades[tickerId] = trade;
    }
    if(field == BID) {
        Quote q;
        q.symbol = "";
        q.bid_price = price;
        m_quotes[tickerId] = q;
    } else if(field == ASK) {
        Quote q;
        q.symbol = "";
        q.ask_price = price;
        m_quotes[tickerId] = q;
    }
}

void TwsApi::orderStatus(OrderId orderId, const std::string& status, Decimal /*filled*/,
    Decimal /*remaining*/, double /*avgFillPrice*/, long long /*permId*/, int /*parentId*/,
    double /*lastFillPrice*/, int /*clientId*/, const std::string& /*whyHeld*/, double /*mktCapPrice*/) {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::string client_order_id = std::to_string(orderId);
    OrderResult result;
    result.id = client_order_id;
    result.status = status;
    m_orders[client_order_id] = result;
    // Signal that order status is updated.
    m_cond.notify_all();
}


void TwsApi::openOrder(OrderId orderId, const Contract& /*contract*/, const Order& order, const OrderState& orderState) {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::string client_order_id = order.orderRef;
    OrderResult result;
    result.id = client_order_id;
    result.status = orderState.status;
    m_orders[client_order_id] = result;
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
    std::unique_lock<std::mutex> lock(m_mutex);
    // ANSI escape code for green text: "\033[32m"
    // Reset code: "\033[0m"
    std::cerr << "\033[32mError (id " << id << ") at " << std::ctime(&errorTime)
              << "Code: " << errorCode << " - " << errorString << "\033[0m" << std::endl;
    m_cond.notify_all();
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
    pos.symbol = contract.symbol;
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
void TwsApi::openOrderEnd() {
    // Empty implementation for now.
}
