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

std::string DecimalFunctions::decimalToString(Decimal value) {
    char buf[64];  // Assuming 64 bytes is enough for the decimal representation
    unsigned int status = 0;
    __bid64_to_string(buf, value, &status);

    // Check for conversion errors
    if (status != 0) {
        return "Conversion Error";
    }

    return std::string(buf);
}

double DecimalFunctions::decimalToDouble(Decimal value) {
    unsigned int status = 0;
    // Use the external function to convert the decimal to double.
    double result = __bid64_to_binary64(value, 0, &status);

    // Check for conversion errors
    if (status != 0) {
        // Optionally, log or handle the error here.
        return 0.0; // Return 0.0 as a fallback in case of an error.
    }

    return result;
}

// Implement other functions if needed...
