//
// Created by m41k1 on 3/6/25.
//
#include "Decimal.h"
#include <vector>
#include <cstring>
#include <string>

Decimal DecimalFunctions::stringToDecimal(std::string str) {
    // Prepare a modifiable buffer from the input string.
    std::vector<char> buf(str.begin(), str.end());
    buf.push_back('\0'); // ensure null-termination
    unsigned int status = 0;
    // Use the external function to convert the string to Decimal.
    Decimal dec = __bid64_from_string(buf.data(), static_cast<unsigned int>(buf.size()), &status);
    // Optionally, you can check 'status' here for conversion errors.
    return dec;
}

// Implement other functions if needed...
