# Interactive Brokers TWS API Project 🌐⚙️📈

This project provides a comprehensive guide for interacting with the Interactive Brokers TWS API using C++. The system leverages TCP/IP protocols and socket communication to interface with the TWS client, ensuring efficient and real-time data handling. 🚀💡🔧

## Key Features 🎯📊📚

- Establishes robust TCP/IP connections to the TWS client using socket programming.
- Facilitates the submission and management of stock and option orders.
- Enables retrieval of real-time quotes, trade data, and historical market information.
- Manages financial positions and account details.
- Utilizes asynchronous message processing to optimize data handling and system performance.

## Prerequisites ⚒️🔍💻

To build and run this project, ensure the following are installed:

- A C++ compiler (e.g., GCC or Clang).
- CMake for build configuration.
- Interactive Brokers TWS or IB Gateway.
- The Interactive Brokers TWS API C++ SDK.

## Installation Guide 🧩🚀🔄

### 1. **Install TWS or IB Gateway**
Download and install either the Trader Workstation (TWS) or the IB Gateway from the following links:

- [Trader Workstation (TWS)](https://www.interactivebrokers.com/en/trading/tws.php#tws-software)
- [IB Gateway](https://www.interactivebrokers.com/en/trading/ibgateway-stable.php)

> **Note:** TWS provides a graphical interface, while IB Gateway is headless. Both work equally well with the API.

### 2. **Install the TWS API**
Download the latest version of the TWS API from [Interactive Brokers GitHub](https://interactivebrokers.github.io/#).

> **Important:** This project uses the version released on **February 10, 2025**. Using an older version may result in compatibility issues.

- Follow the instructions in the "Download the TWS API" section of the documentation.
- The TWS API relies on an Intel library for mathematical operations. Instructions for installing this library can be found in the `Intel_lib_build.txt` file, located in the `cppclient` directory.
- After successful installation, you should have two files:
    - `libbid.a`
    - `libbid.so`

> Place these files inside a `lib` directory at the same level as the `client` directory within `cppclient`.

### 3. **Build `libTwsSocketClient.so`**
- Navigate to the `client` directory and build the provided Makefile to generate the shared object file:

```bash
cd cppclient/client
make
```

> This should produce the `libTwsSocketClient.so` file.

- Update the paths in your `CMakeLists.txt` to correctly reference the location of your `.so` files and other dependencies.

### 4. **Clone the Project Repository**
```bash
git clone <repository-url>
cd <project-directory>
```

### 5. **Configure the Build Environment**
```bash
mkdir build
cd build
cmake ..
```

### 6. **Compile the Project**
```bash
make
```

### 7. **Execute the Application**
```bash
./TwsApiApp
```

## Functionality Overview 🔍📂⚙️

### 1. **Connecting to TWS**
- **Method:** `bool connect(const std::string& host, int port, int clientId);`
- **Purpose:** Initializes a TCP/IP connection to the TWS client for data exchange.

### 2. **Submitting a Stock Order**
- **Method:** `OrderResult submit_order_stock(...)`
- **Purpose:** Places an order for a stock, accepting parameters like symbol, quantity, order type, and pricing details.

### 3. **Submitting an Option Order**
- **Method:** `OrderResult submit_order_option(...)`
- **Purpose:** Facilitates the submission of option orders, with additional configuration for bracket orders.

### 4. **Listing Existing Orders**
- **Method:** `std::vector<OrderResult> list_orders(...);`
- **Purpose:** Fetches a filtered list of current orders based on user-defined criteria.

### 5. **Cancelling an Order**
- **Method:** `void cancel_order(OrderId order_id);`
- **Purpose:** Cancels an active order using its unique order ID.

### 6. **Retrieving Position Data**
- **Method:** `std::vector<Position> list_positions();`
- **Purpose:** Returns details of current positions held within the account.

### 7. **Fetching Historical Data**
- **Method:** `std::vector<HistoricalBar> get_historical_data_stocks(...);`
- **Purpose:** Requests historical market data for a specific stock within a defined timeframe.

## TCP/IP Communication 🌐📡🔄

The core of this project relies on TCP/IP communication established via the `EClientSocket` from the TWS API. This ensures seamless real-time data transmission and transaction processing between the application and TWS. 💬⚡🔒

## Asynchronous Message Processing 🔄⚙️🛠️

- The system employs `EReaderOSSignal` to facilitate efficient and asynchronous message handling.
- Thread safety is maintained through the use of mutexes and condition variables, ensuring consistent data integrity during concurrent operations.

## Contribution Guidelines 🤝📬💡

Contributions are welcome! If you wish to propose a significant change, kindly open an issue first to discuss your ideas. Pull requests should adhere to the existing coding conventions and be well-documented. ✅📝🔍

## License Information 📜🔒✅

This project is licensed under the MIT License. For more details, refer to the LICENSE file. 📄🔍📚

