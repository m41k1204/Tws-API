#include "TwsApi.h"
#include <iostream>
#include <string>
#include <vector>

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
        std::cout << "7: Obtener cotizaciones de acciones" << std::endl;
        std::cout << "8: Obtener cotizaciones de opciones" << std::endl;
        std::cout << "9: Obtener trades de acciones" << std::endl;
        std::cout << "10: Obtener trades de opciones" << std::endl;
        std::cout << "11: Modificar orden" << std::endl;
        std::cout << "12: Consultar orden" << std::endl;
        std::cout << "13: Obtener datos históricos para acciones" << std::endl;
        std::cout << "0: Salir" << std::endl;
        std::cout << "Ingrese una opción: ";

        int opcion = 0;
        std::cin >> opcion;

        switch (opcion) {
            case 1: { // Enviar orden de acciones
                std::string simbolo, lado, tipo, tif, idOrdenCliente;
                int cantidad;
                double precioLimite, precioStop;
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
                std::cout << "Ingrese el precio de stop (0 si no aplica): ";
                std::cin >> precioStop;
                std::cout << "Ingrese el id de orden asignado por el cliente: ";
                std::cin >> idOrdenCliente;

                OrderResult resultado = api.submit_order_stock(simbolo, cantidad, lado, tipo, tif, precioLimite, precioStop, idOrdenCliente);
                std::cout << "Orden de acciones enviada: id = " << resultado.id << ", estado = " << resultado.status << std::endl;
                break;
            }
            case 2: { // Enviar orden de opciones
                std::string simbolo, lado, tipo, tif, idOrdenCliente;
                int cantidad;
                double precioLimite, precioStop, takeProfit, stopLoss;
                std::cout << "Ingrese el símbolo de la opción: ";
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
                std::cout << "Ingrese el precio de stop (0 si no aplica): ";
                std::cin >> precioStop;
                std::cout << "Ingrese el id de orden asignado por el cliente: ";
                std::cin >> idOrdenCliente;
                std::cout << "Ingrese el precio de take profit (0 si no aplica): ";
                std::cin >> takeProfit;
                std::cout << "Ingrese el precio de stop loss (0 si no aplica): ";
                std::cin >> stopLoss;

                OrderResult resultado = api.submit_order_option(simbolo, cantidad, lado, tipo, tif, precioLimite, precioStop, idOrdenCliente, takeProfit, stopLoss);
                std::cout << "Orden de opciones enviada: id = " << resultado.id << ", estado = " << resultado.status << std::endl;
                break;
            }
            case 3: { // Listar órdenes
                std::string estado, despues, hasta, direccion, simbolos, filtroLado;
                int limite;
                std::cout << "Ingrese el estado de las órdenes (open/closed/all): ";
                std::cin >> estado;
                std::cout << "Ingrese el límite: ";
                std::cin >> limite;
                std::cout << "Ingrese 'después' (timestamp, o dejar vacío): ";
                std::cin.ignore(); // Limpiar el salto de línea
                std::getline(std::cin, despues);
                std::cout << "Ingrese 'hasta' (timestamp, o dejar vacío): ";
                std::getline(std::cin, hasta);
                std::cout << "Ingrese la dirección (asc/desc): ";
                std::cin >> direccion;
                std::cout << "Ingrese los símbolos (separados por comas, o dejar vacío): ";
                std::cin >> simbolos;
                std::cout << "Ingrese el filtro de lado (buy/sell, o dejar vacío): ";
                std::cin >> filtroLado;

                std::vector<OrderResult> ordenes = api.list_orders(estado, limite, despues, hasta, direccion, simbolos, filtroLado);
                for (const auto& orden : ordenes)
                    std::cout << "Orden: id = " << orden.id << ", estado = " << orden.status << std::endl;
                break;
            }
            case 4: { // Cancelar orden
                std::string idOrden;
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
                std::string simbolos, moneda;
                std::cout << "Ingrese los símbolos de las acciones (separados por comas): ";
                std::cin >> simbolos;
                std::cout << "Ingrese la moneda (ej.: USD): ";
                std::cin >> moneda;
                std::vector<Quote> cotizaciones = api.get_latest_quotes_stocks(simbolos, moneda);
                for (const auto& cotizacion : cotizaciones)
                    std::cout << "Cotización para " << cotizacion.symbol << ": Bid = " << cotizacion.bid_price << ", Ask = " << cotizacion.ask_price << std::endl;
                break;
            }
            case 8: { // Obtener cotizaciones de opciones
                std::string simbolos;
                std::cout << "Ingrese los símbolos de las opciones (separados por comas): ";
                std::cin >> simbolos;
                std::vector<Quote> cotizaciones = api.get_latest_quotes_options(simbolos);
                for (const auto& cotizacion : cotizaciones)
                    std::cout << "Cotización de opción para " << cotizacion.symbol << ": Bid = " << cotizacion.bid_price << ", Ask = " << cotizacion.ask_price << std::endl;
                break;
            }
            case 9: { // Obtener trades de acciones
                std::string simbolos, moneda;
                std::cout << "Ingrese los símbolos de las acciones (separados por comas): ";
                std::cin >> simbolos;
                std::vector<Trade> trades = api.get_latest_trades_stocks(simbolos, "");
                for (const auto& trade : trades)
                    std::cout << "Trade para " << trade.symbol << ": Precio = " << trade.trade_price << std::endl;
                break;
            }
            case 10: { // Obtener trades de opciones
                std::string simbolos;
                std::cout << "Ingrese los símbolos de las opciones (separados por comas): ";
                std::cin >> simbolos;
                std::vector<Trade> trades = api.get_latest_trades_options(simbolos);
                for (const auto& trade : trades)
                    std::cout << "Trade de opción para " << trade.symbol << ": Precio = " << trade.trade_price << std::endl;
                break;
            }
            case 11: { // Modificar orden
                std::string idOrdenCliente, tif;
                int cantidad;
                double precioLimite, precioStop;
                std::cout << "Ingrese el id de la orden del cliente: ";
                std::cin >> idOrdenCliente;
                std::cout << "Ingrese la nueva cantidad: ";
                std::cin >> cantidad;
                std::cout << "Ingrese el nuevo tiempo en vigor (DAY/GTC): ";
                std::cin >> tif;
                std::cout << "Ingrese el nuevo precio límite (0 si no se modifica): ";
                std::cin >> precioLimite;
                std::cout << "Ingrese el nuevo precio de stop (0 si no se modifica): ";
                std::cin >> precioStop;
                OrderResult res = api.change_order_by_order_id(idOrdenCliente, cantidad, tif, precioLimite, precioStop);
                std::cout << "Orden modificada: id = " << res.id << ", estado = " << res.status << std::endl;
                break;
            }
            case 12: { // Consultar orden
                std::string idOrden;
                std::cout << "Ingrese el id de la orden del cliente: ";
                std::cin >> idOrden;
                OrderResult res = api.get_order(idOrden);
                std::cout << "Orden: id = " << res.id << ", estado = " << res.status << std::endl;
                break;
            }
            case 13: { // Obtener datos históricos para acciones
                std::string simbolo, inicio, fin;
                int limite;
                std::cout << "Ingrese el símbolo de la acción: ";
                std::cin >> simbolo;
                std::cout << "Ingrese el datetime de inicio: ";
                std::cin >> inicio;
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
