# Aplicación API TWS en C++ - README

## Visión General
Esta aplicación en C++ se conecta a la API de Interactive Brokers Trader Workstation (TWS) y proporciona una interfaz basada en menús para interactuar con diversas operaciones de trading. Permite a los usuarios enviar, modificar, cancelar órdenes, recuperar datos históricos y suscribirse a datos de mercado.

---

## Tabla de Contenidos
1. [Funciones de Conexión](#funciones-de-conexión)
2. [Funciones del Menú](#funciones-del-menú)
3. [Explicación de Funciones](#explicación-de-funciones)
4. [Casos de Uso](#casos-de-uso)

---

## Funciones del Menú (Cases)

### 1. Enviar Orden de Acciones (`api.submit_order_stock`)
- **Descripción**: Envía una orden de compra o venta de acciones.
- **Argumentos**:
    - `symbol` (string): El símbolo de la acción.
    - `quantity` (int): Cantidad de acciones a comprar o vender.
    - `side` (string): Dirección de la orden ("buy" o "sell").
    - `type` (string): Tipo de orden ("market", "limit", "stop", "stop_limit").
    - `tif` (string): Tiempo en vigor ("DAY" o "GTC").
    - `limitPrice` (double): Precio límite de la orden.
    - `stopPrice` (double): Precio de activación para órdenes "stop".
    - `orderId` (string): ID asignado por el cliente.
    - `takeProfitPrice` (double): Precio para toma de ganancias (en ordenes bracket).
    - `stopLossPrice` (double): Precio de stop loss (en ordenes bracket).
    - `isBracket` (bool): Indica si es una orden bracket.
- **Ejemplo de Uso**:
```cpp
api.submit_order_stock("AAPL", 100, "buy", "limit", "DAY", 150.0, 0.0, "order123", 160.0, 140.0, true);
```

### 2. Enviar Orden de Opciones (`api.submit_order_option`)
- **Descripción**: Envía una orden para la compra o venta de opciones.
- **Argumentos**: Mismos que `submit_order_stock`.
- **Ejemplo de Uso**:
```cpp
api.submit_order_option("AAPL_20230616C150", 10, "buy", "market", "GTC", 0.0, 0.0, "optOrder456", 0.0, 0.0, false);
```

### 3. Listar Órdenes (`api.list_orders`)
- **Descripción**: Muestra una lista de órdenes filtradas.
- **Argumentos**:
    - `state` (string): Estado de la orden.
    - `limit` (int): Límite de registros.
    - `after` (string): Filtrar por órdenes después de una fecha.
    - `until` (string): Filtrar por órdenes antes de una fecha.
    - `direction` (string): "asc" o "desc".
    - `symbols` (string): Símbolos a filtrar o "all".
    - `side` (string): "buy" o "sell".
- **Ejemplo de Uso**:
```cpp
api.list_orders("submitted", 10, "", "", "asc", "AAPL", "buy");
```

### 4. Cancelar Orden (`api.cancel_order`)
- **Descripción**: Cancela una orden por su ID.
- **Argumentos**:
    - `orderId` (int): ID de la orden a cancelar.
- **Ejemplo de Uso**:
```cpp
api.cancel_order(12345);
```

### 5. Listar Posiciones (`api.list_positions`)
- **Descripción**: Lista todas las posiciones actuales.
- **Ejemplo de Uso**:
```cpp
api.list_positions();
```

### 6. Obtener Posición para un Símbolo (`api.get_position`)
- **Descripción**: Recupera la posición para un símbolo.
- **Argumentos**:
    - `symbol` (string): El símbolo de la acción.
- **Ejemplo de Uso**:
```cpp
api.get_position("AAPL");
```

### 7. Subscribir Cotizaciones de Acciones (`api.subscribe_stock_quotes`)
- **Descripción**: Se suscribe a cotizaciones en tiempo real.
- **Argumentos**:
    - `symbols` (string): Símbolos separados por comas.
- **Ejemplo de Uso**:
```cpp
api.subscribe_stock_quotes("AAPL,MSFT,GOOG");
```

### 8. Subscribir Cotizaciones de Opciones (`api.subscribe_option_quotes`)
- **Descripción**: Se suscribe a cotizaciones en tiempo real de opciones.
- **Ejemplo de Uso**:
```cpp
api.subscribe_option_quotes("AAPL_20230616C150,AAPL_20230616P140");
```

### 9. Subscribir Trades de Acciones (`api.subscribe_stock_trades`)
- **Descripción**: Se suscribe a los datos de operaciones en tiempo real.
- **Ejemplo de Uso**:
```cpp
api.subscribe_stock_trades("AAPL,MSFT");
```

### 10. Subscribir Trades de Opciones (`api.subscribe_option_trades`)
- **Descripción**: Se suscribe a operaciones en tiempo real para opciones.
- **Ejemplo de Uso**:
```cpp
api.subscribe_option_trades("AAPL_20230616C150");
```

### 11. Modificar Orden (`api.change_order_by_order_id`)
- **Descripción**: Modifica los parámetros de una orden.
- **Argumentos**:
    - `orderId` (int): ID de la orden.
    - `quantity` (int): Nueva cantidad.
    - `tif` (string): Nuevo tiempo en vigor.
    - `limitPrice` (double): Nuevo precio límite.
    - `stopPrice` (double): Nuevo precio stop.
- **Ejemplo de Uso**:
```cpp
api.change_order_by_order_id(12345, 200, "GTC", 155.0, 0.0);
```

### 12. Obtener Datos Históricos para Acciones (`api.get_historical_data_stocks`)
- **Descripción**: Obtiene datos históricos para un símbolo.
- **Argumentos**:
    - `symbol` (string): Símbolo.
    - `start` (string): Fecha de inicio.
    - `end` (string): Fecha de fin.
    - `limit` (int): Límite de registros.
- **Ejemplo de Uso**:
```cpp
api.get_historical_data_stocks("AAPL", "2023-01-01", "2023-01-31", 100);
```

---

Este documento sirve como una guía detallada para entender y utilizar las funciones de la API de TWS implementadas en esta aplicación en C++.

