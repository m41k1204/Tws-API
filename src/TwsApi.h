#ifndef TWS_API_H
#define TWS_API_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <unordered_map>

#include "EWrapper.h"
#include "EClientSocket.h"
#include "EReaderOSSignal.h"  // Added to support asynchronous message processing
#include "Contract.h"
#include "Order.h"
#include "Decimal.h"

struct OrderResult {
    OrderId orderId = 0;
    std::string orderRef = "";
    std::string status = "";
    std::string symbol = "";
    std::string side = "";
    int qty = 0;
    std::string orderType = "";
    std::string tif = "";
    double limit_price = 0.0;
    double stop_price = 0.0;
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::time_point{};
};


struct Position {
    std::string symbol;
    int qty;
    double avgCost;
};

struct Quote {
    std::string symbol;
    double bid_price;
    double ask_price;
};

struct Trade {
    std::string symbol;
    double trade_price;
    time_t timestamp;  // Timestamp of the trade
};


struct HistoricalBar {
    std::string time;
    double open;
    double high;
    double low;
    double close;
    long volume;
};

class TwsApi : public EWrapper {
public:
    TwsApi();
    virtual ~TwsApi();

    // Connection management
    bool connect(const std::string& host, int port, int clientId);
    void disconnect();

    // Order functions (stocks and options)
    OrderResult submit_order_stock(const std::string& symbol, int qty, const std::string& side,
      const std::string& type, const std::string& time_in_force,
      double limit_price, double stop_price, const std::string& client_order_id);

    OrderResult submit_order_option(const std::string& symbol, int qty, const std::string& side,
      const std::string& type, const std::string& time_in_force,
      double limit_price, double stop_price, const std::string& client_order_id,
      double bracket_take_profit_price = 0.0, double bracket_stop_loss_price = 0.0);

    std::vector<OrderResult> list_orders(const std::string& status, int limit,
      const std::string& after, const std::string& until,
      const std::string& direction, const std::string& symbols,
      const std::string& side);

    void cancel_order(OrderId order_id);

    // Position functions
    std::vector<Position> list_positions();
    Position get_position(const std::string& symbol);

    // Market data (quotes and trades)
    std::vector<Quote> get_latest_quotes_stocks(const std::string& symbols, const std::string& currency);
    std::vector<Quote> get_latest_quotes_options(const std::string& symbols);

    std::vector<Trade> get_latest_trades_stocks(const std::string& symbols, const std::string& currency);
    std::vector<Trade> get_latest_trades_options(const std::string& symbols);

    // Order modification and query
    OrderResult change_order_by_order_id(OrderId order_id,
    int qty, std::string time_in_force,
    std::optional<double> limit_price, std::optional<double> stop_price);

    OrderResult get_order(const std::string& order_id);

    // Historical data (for stocks)
    std::vector<HistoricalBar> get_historical_data_stocks(const std::string& symbol,
      const std::string& start, const std::string& end, int limit);

    // NEW: Request all open orders from TWS.
    void reqAllOpenOrders();

    // --- EWrapper callbacks ---
    virtual void tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib) override;
    virtual void tickSize(TickerId tickerId, TickType field, Decimal size) override;
    virtual void tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta,
        double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice) override;
    virtual void tickGeneric(TickerId tickerId, TickType tickType, double value) override;
    virtual void tickString(TickerId tickerId, TickType tickType, const std::string& value) override;
    virtual void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
        double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) override;
    virtual void orderStatus( OrderId orderId, const std::string& status, Decimal filled,
        Decimal remaining, double avgFillPrice, long long permId, int parentId,
        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) override;
    virtual void openOrder( OrderId orderId, const Contract& contract, const Order& order, const OrderState& orderState) override;
    virtual void openOrderEnd() override;  // Make sure this is defined in TwsApi.cpp
    virtual void winError( const std::string& str, int lastError) override;
    virtual void connectionClosed() override;
    virtual void updateAccountValue(const std::string& key, const std::string& val,
        const std::string& currency, const std::string& accountName) override;
    virtual void updatePortfolio( const Contract& contract, Decimal position,
        double marketPrice, double marketValue, double averageCost,
        double unrealizedPNL, double realizedPNL, const std::string& accountName) override;
    virtual void updateAccountTime(const std::string& timeStamp) override;
    virtual void accountDownloadEnd(const std::string& accountName) override;
    virtual void nextValidId( OrderId orderId) override;
    virtual void contractDetails( int reqId, const ContractDetails& contractDetails) override;
    virtual void bondContractDetails( int reqId, const ContractDetails& contractDetails) override;
    virtual void contractDetailsEnd( int reqId) override;
    virtual void execDetails( int reqId, const Contract& contract, const Execution& execution) override;
    virtual void execDetailsEnd( int reqId) override;
    virtual void error(int id, time_t errorTime, int errorCode,const std::string& errorString, const std::string& advancedOrderRejectJson) override;
    virtual void updateMktDepth(TickerId id, int position, int operation, int side,
        double price, Decimal size) override;
    virtual void updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
        int side, double price, Decimal size, bool isSmartDepth) override;
    virtual void updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) override;
    virtual void managedAccounts( const std::string& accountsList) override;
    virtual void receiveFA(faDataType pFaDataType, const std::string& cxml) override;
    virtual void historicalData(TickerId reqId, const Bar& bar) override;
    virtual void historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) override;
    virtual void scannerParameters(const std::string& xml) override;
    virtual void scannerData(int reqId, int rank, const ContractDetails& contractDetails,
        const std::string& distance, const std::string& benchmark, const std::string& projection,
        const std::string& legsStr) override;
    virtual void scannerDataEnd(int reqId) override;
    virtual void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
        Decimal volume, Decimal wap, int count) override;
    virtual void currentTime(long time) override;
    virtual void fundamentalData(TickerId reqId, const std::string& data) override;
    virtual void deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) override;
    virtual void tickSnapshotEnd( int reqId) override;
    virtual void marketDataType( TickerId reqId, int marketDataType) override;
    virtual void commissionAndFeesReport( const CommissionAndFeesReport& commissionAndFeesReport) override;
    virtual void position( const std::string& account, const Contract& contract, Decimal position, double avgCost) override;
    virtual void positionEnd() override;
    virtual void accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) override;
    virtual void accountSummaryEnd( int reqId) override;
    virtual void verifyMessageAPI( const std::string& apiData) override;
    virtual void verifyCompleted( bool isSuccessful, const std::string& errorText) override;
    virtual void displayGroupList( int reqId, const std::string& groups) override;
    virtual void displayGroupUpdated( int reqId, const std::string& contractInfo) override;
    virtual void verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange) override;
    virtual void verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) override;
    virtual void connectAck() override;
    virtual void positionMulti( int reqId, const std::string& account, const std::string& modelCode, const Contract& contract, Decimal pos, double avgCost) override;
    virtual void positionMultiEnd( int reqId) override;
    virtual void accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) override;
    virtual void accountUpdateMultiEnd( int reqId) override;
    virtual void securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
        const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes) override;
    virtual void securityDefinitionOptionalParameterEnd(int reqId) override;
    virtual void softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) override;
    virtual void familyCodes(const std::vector<FamilyCode> &familyCodes) override;
    virtual void symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) override;
    virtual void mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) override;
    virtual void tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData) override;
    virtual void smartComponents(int reqId, const SmartComponentsMap& theMap) override;
    virtual void tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) override;
    virtual void newsProviders(const std::vector<NewsProvider> &newsProviders) override;
    virtual void newsArticle(int requestId, int articleType, const std::string& articleText) override;
    virtual void historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline) override;
    virtual void historicalNewsEnd(int requestId, bool hasMore) override;
    virtual void headTimestamp(int reqId, const std::string& headTimestamp) override;
    virtual void histogramData(int reqId, const HistogramDataVector& data) override;
    virtual void historicalDataUpdate(TickerId reqId, const Bar& bar) override;
    virtual void rerouteMktDataReq(int reqId, int conid, const std::string& exchange) override;
    virtual void rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) override;
    virtual void marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) override;
    virtual void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) override;
    virtual void pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) override;
    virtual void historicalTicks(int reqId, const std::vector<HistoricalTick> &ticks, bool done) override;
    virtual void historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done) override;
    virtual void historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done) override;
    virtual void tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions) override;
    virtual void tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) override;
    virtual void tickByTickMidPoint(int reqId, time_t time, double midPoint) override;
    virtual void orderBound(long long permId, int clientId, int orderId) override;
    virtual void completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) override;
    virtual void completedOrdersEnd() override;
    virtual void replaceFAEnd(int reqId, const std::string& text) override;
    virtual void wshMetaData(int reqId, const std::string& dataJson) override;
    virtual void wshEventData(int reqId, const std::string& dataJson) override;
    virtual void historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime, const std::string& timeZone, const std::vector<HistoricalSession>& sessions) override;
    virtual void userInfo(int reqId, const std::string& whiteBrandingId) override;
    virtual void currentTimeInMillis(time_t timeInMillis) override;

public:
    EClientSocket* m_client;
    EReaderOSSignal* m_signal; // Added signal for asynchronous processing
    int m_nextOrderId;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::map<OrderId, OrderResult> m_orders;  // Keyed by client_order_id
    std::vector<Position> m_positions;
    std::map<TickerId, Quote> m_quotes;
    std::map<TickerId, Trade> m_trades;
    std::map<int, std::vector<HistoricalBar>> m_historicalData;  // Keyed by request id
    std::unordered_map<int, std::string> m_reqIdToSymbol;

    // Helper functions to build IB contracts
    Contract createStockContract(const std::string& symbol);
    Contract createOptionContract(const std::string& symbol);
};

#endif // TWS_API_H
