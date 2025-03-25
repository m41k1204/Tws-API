#include <iomanip>

#include "TwsApi.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>

int main() {
    TwsApi api;

    std::cout << "Conectando a TWS..." << std::endl;
    if(api.connect("127.0.0.1", 7497, 0)) {
        if(api.m_client->isConnected()) {
            std::cout << "Successfully connected to TWS. " << std::endl;
        } else {
            std::cerr << "Not connected, but connect() returned true?" << std::endl;
        }
    }

    bool ejecutando = true;
    while (ejecutando) {
        std::cout << "\n===== Menú de la API de TWS =====" << std::endl;
        std::cout << "1: Enviar orden de acciones" << std::endl;
        std::cout << "2: Enviar orden de opciones" << std::endl;
        std::cout << "3: Listar órdenes" << std::endl;
        std::cout << "4: Cancelar orden" << std::endl;
        std::cout << "5: Listar posiciones" << std::endl;
        std::cout << "6: Obtener posición para un símbolo" << std::endl;
        std::cout << "7: Subscribir cotizaciones de acciones" << std::endl;
        std::cout << "8: Subscribir cotizaciones de opciones" << std::endl;
        std::cout << "9: Subscribir trades de acciones" << std::endl;
        std::cout << "10: Subscribir trades de opciones" << std::endl;
        std::cout << "11: Modificar orden" << std::endl;
        std::cout << "12: Obtener datos históricos para acciones" << std::endl;
        std::cout << "13: Recibir data del mercado" << std::endl;
        std::cout << "14: Filtrar trades por simbolo y tiempo" << std::endl;
        std::cout << "15: Filtrar quotes por simbolo y tiempo" << std::endl;
        std::cout << "16: Recibir data de mercado para opciones (IV y OI)" << std::endl;
        std::cout << "17: Recibir cash amount" << std::endl;
        std::cout << "0: Salir" << std::endl;
        std::cout << "Ingrese una opción: ";

        int opcion = 0;
        std::cin >> opcion;

        switch (opcion) {
            case 1: { // Enviar orden de acciones
            std::string simbolo, lado, tipo, tif, idOrdenCliente;
            int cantidad;
            double precioLimite, precioStop, precioTakeProfit, precioStopLoss;
            char esBracket;

            std::cout << "Ingrese el símbolo de la acción: ";
            std::cin >> simbolo;
            std::cout << "Ingrese la cantidad: ";
            std::cin >> cantidad;
            std::cout << "Ingrese el lado (buy/sell): ";
            std::cin >> lado;
            std::cout << "Ingrese el tipo de orden (market/limit/stop/stop_limit): ";
            std::cin >> tipo;
            std::cout << "Ingrese el tiempo en vigor (DAY/GTC): ";
            std::cin >> tif;
            std::cout << "Ingrese el precio límite (0 si no aplica): ";
            std::cin >> precioLimite;
            std::cout << "Ingrese el precio stop (0 si no aplica): ";
            std::cin >> precioStop;
            std::cout << "Ingrese el id de orden asignado por el cliente: ";
            std::cin >> idOrdenCliente;
            std::cout << "¿Es una orden bracket? (S/N): ";
            std::cin >> esBracket;

            bool is_bracket = (esBracket == 'S' || esBracket == 's');

            if (is_bracket) {
                std::cout << "Ingrese el precio de take profit: ";
                std::cin >> precioTakeProfit;
                std::cout << "Ingrese el precio de stop loss: ";
                std::cin >> precioStopLoss;
            }

            OrderResult resultado = api.submit_order_stock(simbolo, cantidad, lado, tipo, tif, precioLimite, precioStop,
                                                      idOrdenCliente, precioTakeProfit, precioStopLoss, is_bracket);

            api.printOrderResult(resultado);
                break;
        }

        case 2: { // Enviar orden de opciones
                std::string simbolo, lado, tipo, tif, idOrdenCliente;
                int cantidad;
                double precioLimite, precioStop, precioTakeProfit, precioStopLoss;
                char esBracket;

                std::cout << "Ingrese el símbolo de la acción: ";
                std::cin >> simbolo;
                std::cout << "Ingrese la cantidad: ";
                std::cin >> cantidad;
                std::cout << "Ingrese el lado (buy/sell): ";
                std::cin >> lado;
                std::cout << "Ingrese el tipo de orden (market/limit/stop/stop_limit): ";
                std::cin >> tipo;
                std::cout << "Ingrese el tiempo en vigor (DAY/GTC): ";
                std::cin >> tif;
                std::cout << "Ingrese el precio límite (0 si no aplica): ";
                std::cin >> precioLimite;
                std::cout << "Ingrese el precio stop (0 si no aplica): ";
                std::cin >> precioStop;
                std::cout << "Ingrese el id de orden asignado por el cliente: ";
                std::cin >> idOrdenCliente;
                std::cout << "¿Es una orden bracket? (S/N): ";
                std::cin >> esBracket;

                bool is_bracket = (esBracket == 'S' || esBracket == 's');

                if (is_bracket) {
                    std::cout << "Ingrese el precio de take profit: ";
                    std::cin >> precioTakeProfit;
                    std::cout << "Ingrese el precio de stop loss: ";
                    std::cin >> precioStopLoss;
                }

            OrderResult resultado = api.submit_order_option(simbolo, cantidad, lado, tipo, tif, precioLimite, precioStop,
                                                            idOrdenCliente, precioTakeProfit, precioStopLoss, is_bracket);
                api.printOrderResult(resultado);
                break;
}
            case 3: { // Listar órdenes
                std::string estado, despues, hasta, direccion, simbolos, filtroLado;
                int limite;
                std::cout << "Ingrese el límite: ";
                std::cin >> limite;
                std::cout << "Ingrese la dirección (asc/desc): ";
                std::cin >> direccion;
                std::cout << "Ingrese los símbolos (separados por comas, (all para todos) ): ";
                std::cin >> simbolos;
                if (simbolos == "all") {simbolos = "";}
                std::cout << "Ingrese el filtro de lado (BUY/SELL, (all para todos)): ";
                std::cin >> filtroLado;
                if (filtroLado == "all") {filtroLado = "";}

                api.reqAllOpenOrders();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                std::vector<OrderResult> ordenes = api.list_orders(estado, limite, despues, hasta, direccion, simbolos, filtroLado);
                std::cout << "\nOrdenes:\n\n";

                // Print header row with fixed width columns.
                std::cout << std::left
                          << std::setw(10) << "OrderID"
                          << std::setw(15) << "OrderRef"
                          << std::setw(22) << "Symbol"
                          << std::setw(15) << "Status"
                          << std::setw(10) << "Side"
                          << std::setw(10) << "Quantity"
                          << std::setw(12) << "OrderType"
                          << std::setw(8)  << "TIF"
                          << std::setw(12) << "Limit Price"
                          << std::setw(12) << "Stop Price"
                          << std::endl;

                std::cout << std::string(110, '-') << std::endl;

                for (const auto& order : ordenes) {
                    std::cout << std::left
                              << std::setw(10) << order.orderId
                              << std::setw(15) << order.orderRef
                              << std::setw(22) << order.symbol
                              << std::setw(15) << order.status
                              << std::setw(12) << order.side
                              << std::setw(10) << order.qty
                              << std::setw(10) << order.orderType
                              << std::setw(12)  << order.tif
                              << std::setw(12) << order.limit_price
                              << std::setw(12) << order.stop_price
                              << std::endl;
                }
                break;
            }
            case 4: { // Cancelar orden
                OrderId idOrden;
                std::cout << "Ingrese el id de la orden a cancelar: ";
                std::cin >> idOrden;
                api.cancel_order(idOrden);
                std::cout << "Orden cancelada." << std::endl;
                break;
            }
            case 5: { // Listar posiciones
                std::vector<Position> posiciones = api.list_positions();
                for (const auto& pos : posiciones)
                    std::cout << "Posición: símbolo = " << pos.symbol << ", cantidad = " << pos.qty << ", costo promedio = " << pos.avgCost << std::endl;
                break;
            }
            case 6: { // Obtener posición para un símbolo
                std::string simbolo;
                std::cout << "Ingrese el símbolo: ";
                std::cin >> simbolo;
                Position pos = api.get_position(simbolo);
                std::cout << "Posición para " << simbolo << ": cantidad = " << pos.qty << ", costo promedio = " << pos.avgCost << std::endl;
                break;
            }
            case 7: { // Obtener cotizaciones de acciones
                std::string simbolos;
                std::cout << "Ingrese los símbolos de las acciones (separados por comas): ";
                std::cin >> simbolos;
                api.subscribe_stock_quotes(simbolos);
            }
            case 8: { // Obtener cotizaciones de opciones
                std::string simbolos;
                std::cout << "Ingrese los símbolos de las opciones (separados por comas): ";
                std::cin >> simbolos;
                api.subscribe_option_quotes(simbolos);
            }
            case 9: { // Obtener trades de acciones
                std::string simbolos;
                std::cout << "Ingrese los símbolos de las acciones (separados por comas): ";
                std::cin >> simbolos;
                api.subscribe_stock_trades(simbolos);
            }
            case 10: { // Obtener trades de opciones
                std::string simbolos;
                std::cout << "Ingrese los símbolos de las opciones (separados por comas): ";
                std::cin >> simbolos;
                api.subscribe_option_trades(simbolos);
            }
            case 11: {
                // Modificar orden
                OrderId order_id;
                std::string  tif;
                int cantidad;
                double precioLimite, precioStop;
                std::cout << "Ingrese el id de la orden: ";
                std::cin >> order_id;
                std::cout << "Ingrese la nueva cantidad ( 0 si no aplica ) :";
                std::cin >> cantidad;
                std::cout << "Ingrese el nuevo tiempo en vigor (DAY/GTC) ( - si no aplica ) :";
                std::cin >> tif;
                std::cout << "Ingrese el nuevo precio límite (0 si no se modifica): ";
                std::cin >> precioLimite;
                std::cout << "Ingrese el nuevo precio de stop (0 si no se modifica): ";
                std::cin >> precioStop;

                api.reqAllOpenOrders();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                OrderResult res = api.change_order_by_order_id(order_id, cantidad, tif, precioLimite, precioStop);
                std::cout << "Orden modificada: id = " << res.orderId << ", estado = " << res.status << std::endl;
                break;
            }
            case 12: {
                // Obtener datos históricos para acciones
                std::string simbolo, inicio, fin;
                int limite;
                std::cout << "Ingrese el símbolo de la acción: ";
                std::cin >> simbolo;
                std::cout << "Ingrese el datetime de fin: ";
                std::cin >> fin;
                std::cout << "Ingrese el límite de registros: ";
                std::cin >> limite;
                std::vector<HistoricalBar> barras = api.get_historical_data_stocks(simbolo, inicio, fin, limite);
                for (const auto& barra : barras) {
                    std::cout << "Barra histórica en " << barra.time << ": O = " << barra.open
                              << ", H = " << barra.high << ", L = " << barra.low
                              << ", C = " << barra.close << ", Volumen = " << barra.volume << std::endl;
                }
                break;
            }
            case 13: { // Request market data
                std::string symbol;
                std::cout << "Ingrese el simbolo: ";
                std::cin >> symbol;
                api.requestMarketData(symbol);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                // Display latest quote for this symbol
                for (const auto& [tickerId, quote] : api.m_quotes) {
                    if (quote.symbol == symbol) {
                        std::cout << "Data: " << symbol << ": Bid = " << quote.bid_price
                                  << ", Ask = " << quote.ask_price << ", Last = " << quote.last_price
                                  << ", Close = " << quote.close_price << std::endl;
                    }
                }
                api.cancelMarketData(api.m_nextTickerId - 1);
                break;
            }
            case 14: {
                std::string inputSimbolos;
                int segundos;
                std::cout << "Ingrese los símbolos (separados por comas): ";
                std::cin >> inputSimbolos;
                std::cout << "Ingrese el intervalo de tiempo en segundos: ";
                std::cin >> segundos;

                std::vector<Trade> tradesRecientes = api.filterTradesForLastSeconds(inputSimbolos, segundos);
                std::cout << "Número de trades recientes: " << tradesRecientes.size() << std::endl;
                for (const auto& t : tradesRecientes) {
                    TwsApi::printTradeInline(t);
                }
                break;
            }
            case 15: {
                std::string inputSimbolos;
                int segundos;
                std::cout << "Ingrese los símbolos (separados por comas): ";
                std::cin >> inputSimbolos;
                std::cout << "Ingrese el intervalo de tiempo en segundos: ";
                std::cin >> segundos;

                std::vector<Quote> quotesRecientes = api.filterQuotesForLastSeconds(inputSimbolos, segundos);
                std::cout << "Número de cotizaciones recientes: " << quotesRecientes.size() << std::endl;
                for (const auto& q : quotesRecientes) {
                    TwsApi::printQuoteInline(q);
                }
                break;
            }
            case 16: {
                std::string optionSymbol;
                std::cout << "Ingrese el símbolo de la opción (ej: AAPL240419C00170000): ";
                std::cin >> optionSymbol;

                OptionQuote quote = api.getOptionQuote(optionSymbol);

                std::cout << std::fixed << std::setprecision(4)
                          << "\n--- Datos recibidos para la opción ---\n"
                          << "Símbolo: " << optionSymbol << "\n"
                          << "Bid Price: " << quote.bidPrice << "\n"
                          << "Ask Price: " << quote.ask_price << "\n"
                          << "Volume: " << quote.volume << "\n"
                          << "Implied Volatility: " << quote.impliedVolatility << "\n";
                break;
            }
            case 17: {
                double cash_amount = api.getCashBalance();
                std::cout << "Cash: " << cash_amount << std::endl;
                break;
            }
            case 0:
                ejecutando = false;
                break;
            default:
                std::cout << "Opción inválida." << std::endl;
        }
    }

    api.disconnect();
    std::cout << "Desconectado de TWS. Hasta luego." << std::endl;
    return 0;
}
