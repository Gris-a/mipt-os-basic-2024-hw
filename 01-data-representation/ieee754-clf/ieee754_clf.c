#include <stdint.h>
#include <string.h>

#include "ieee754_clf.h"

float_class_t classify(double x) {
    uint64_t bits;
    memcpy(&bits, &x, sizeof(x));

    uint64_t sign_bit = bits >> 63;
    uint64_t exp      = (bits << 1) >> 53;
    uint64_t mant     = bits & 0x0FFFFFFFFFFFFF;

    if((exp == 0x0) && (mant == 0x0)) {
        return sign_bit ? MinusZero : Zero;
    } else if((exp == 0x07FF) && (mant == 0x0)) {
        return sign_bit ? MinusInf : Inf;
    } else if((exp == 0x07FF) && (mant != 0x0)) {
        return NaN;
    } else if(exp == 0x0) {
        return sign_bit ? MinusDenormal : Denormal;
    } else {
        return sign_bit ? MinusRegular : Regular;
    }
}
