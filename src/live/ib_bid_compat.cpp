#ifdef __APPLE__

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Decimal.h"

namespace {

Decimal DoubleToDecimalBits(double value) {
    Decimal out = 0;
    static_assert(sizeof(out) == sizeof(value), "Decimal and double size mismatch");
    std::memcpy(&out, &value, sizeof(value));
    return out;
}

double DecimalBitsToDouble(Decimal value) {
    double out = 0.0;
    static_assert(sizeof(value) == sizeof(out), "Decimal and double size mismatch");
    std::memcpy(&out, &value, sizeof(out));
    return out;
}

}

extern "C" Decimal __bid64_add(Decimal a, Decimal b, unsigned int, unsigned int*) {
    return DoubleToDecimalBits(DecimalBitsToDouble(a) + DecimalBitsToDouble(b));
}

extern "C" Decimal __bid64_sub(Decimal a, Decimal b, unsigned int, unsigned int*) {
    return DoubleToDecimalBits(DecimalBitsToDouble(a) - DecimalBitsToDouble(b));
}

extern "C" Decimal __bid64_mul(Decimal a, Decimal b, unsigned int, unsigned int*) {
    return DoubleToDecimalBits(DecimalBitsToDouble(a) * DecimalBitsToDouble(b));
}

extern "C" Decimal __bid64_div(Decimal a, Decimal b, unsigned int, unsigned int*) {
    return DoubleToDecimalBits(DecimalBitsToDouble(a) / DecimalBitsToDouble(b));
}

extern "C" Decimal __bid64_from_string(char* str, unsigned int, unsigned int*) {
    if (!str) {
        return DoubleToDecimalBits(0.0);
    }
    char* end = nullptr;
    const double value = std::strtod(str, &end);
    if (end == str) {
        return DoubleToDecimalBits(0.0);
    }
    return DoubleToDecimalBits(value);
}

extern "C" void __bid64_to_string(char* out, Decimal value, unsigned int*) {
    if (!out) {
        return;
    }
    std::snprintf(out, 64, "%.17g", DecimalBitsToDouble(value));
}

extern "C" double __bid64_to_binary64(Decimal value, unsigned int, unsigned int*) {
    return DecimalBitsToDouble(value);
}

extern "C" Decimal __binary64_to_bid64(double value, unsigned int, unsigned int*) {
    return DoubleToDecimalBits(value);
}

#endif
