#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <bit>

// Define this as-needed to enable the run time component class
//define RUNTIME_COMPONENT 1

#if defined(__clang__)
// Use clang syntax attributes
#define _OPTIMIZE_ [[clang::always_inline]]
#elif defined(__GNUC__)||defined(__GNUG__)
#define _OPTIMIZE_ [[gnu::always_inline]]
#elif defined (_MSC_VER)
#define _OPTIMIZE_ [[msvc::forceinline]]
#else
// Unknown compiler, don't try to optimize anything
#define _OPTIMIZE_
#endif

#define _ALWAYS_CONSTEXPR_ constexpr
// constexpr
#define _CONSTEXPR_ constexpr

// This is a hack to create constants in a user-friendly way, I don't like it
// much
#define _(TXT) Qty<TXT> { 1.0 }

enum UnitError {
    NoError = 0,
    InvalidDefinition,
    BadParenthesis,
    DefinitionTooLong,
    TooManyTokens
};

static _CONSTEXPR_ const char *UnitErrorMessages[] = {
    "OK",
    "Invalid Unit Name",
    "Invalid Exponent",
    "Bad Parenthesis open/close count",
    "Unit definition exceeds maximum length, increase maxDefinitionLength",
    "Unit definition exceeds maximum number of tokens, increase maxTokens"

};

// Configuration constants
constexpr size_t maxTokens = 20;
constexpr size_t maxTokenLength = 10;
constexpr size_t maxDefinitionLength = maxTokens * maxTokenLength / 2;

// String literals as templates for complex units
template <size_t N> struct UTxt {
    constexpr UTxt(const char (&str)[N]) { std::copy_n(str, N, value); }
    char value[N] = {};
};

// Basic math because we cannot use library functions within consteval
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ int64_t gcd(int64_t a, int64_t b) {
    while (b) {
        auto t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// Some compilers are behind on making std::pow constexpr, so here's our own implementation
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ double intpow(double number,
                                                    int64_t exp) {
    long double result = 1;
    long double ldnumber = number;
    if(exp==0) return 1.0;
    if(exp<0) {
        ldnumber = 1.0/ldnumber;
        exp = -exp;
    }
    while (exp>1) {
        if (exp & 1) {
            result *= ldnumber;
        }
        ldnumber *= ldnumber;
        exp >>= 1;
    }
    return result*ldnumber;
}

// For the same reason, we need fractional exponents
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ double introot(double number,
                                                    int64_t nth) {
    // Make sure that the root is a positive integer AND that number is also positive
    // This is true for unit coefficients, so we won't check for that here for speed reasons

    int64_t bitmask=1;
    int64_t log2=0;

    while(number>bitmask && log2<64) {
        bitmask<<=1;
        log2++;
    }

    // Initial guess for Newton-Raphson
    double xnext = 1LL+ (1LL<<(1+(log2/nth)));
    double xprev = number;
    double xrepeat= number;
    int iterations = 0;
    do {
        xrepeat = xprev;
        xprev = xnext;
        xnext = ((nth-1)*xprev+number/intpow(xprev,nth-1))/nth;
        ++iterations;
    } while(xprev!=xnext && xnext!=xrepeat && iterations<200);

    if(iterations == 200 ) {
        // 100 iterations is plenty for the 100th root of any integer up to 2^63
        // For roots more than 100 this will limit the quality of the result (but another method should be used!)
        throw "introot exceeded number of iterations";
    }
    return xnext;
}

// Apparently, also std::frexp and std::ldexp are not constexpr yet on C++20, so here's our own implementation as well
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ double dbl_mantissa(double value) {
    uint64_t bitconversion = std::bit_cast<uint64_t,double>(value);
    bitconversion &= ~0x7ff0'0000'0000'0000ULL;    // Clear the exponent
    bitconversion |=  0x3ff0'0000'0000'0000ULL;    // Set the exponent to 1023 --> 1023 - bias == 0
    return std::bit_cast<double,uint64_t>(bitconversion);
}
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ int dbl_exponent(double value) {
    uint64_t bitconversion = std::bit_cast<uint64_t,double>(value);
    bitconversion &= 0x7ff0'0000'0000'0000ULL;    // Isolate the exponent bits
    bitconversion >>= 52;
    return static_cast<int>(bitconversion-1023);
}
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ double dbl_make(double mantissa, int exponent) {
    uint64_t bitconversion = std::bit_cast<uint64_t,double>(mantissa);

    bitconversion &= ~0x7ff0'0000'0000'0000ULL;    // Clear the exponent bits
    exponent+=1023;
    exponent&=0x7ff;
    bitconversion |= ((uint64_t)exponent)<<52;
    return std::bit_cast<double,uint64_t>(bitconversion);
}

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ size_t outputDigitsNoExponent(int64_t number, char *output, size_t buffersize) {
    size_t idx=0;

    // Find the first digit to output
    int64_t powerOf10=10;
    if(buffersize<2) {
        // Cause a compile error
        throw "Internal text buffer overflow - increase internal buffers";
    }

    if(number<0) {
        output[idx++]='-';
        number=-number;
    }
    while(number>=powerOf10) {
        powerOf10*=10;
        if(powerOf10 == 1'000'000'000'000'000'000LL) {
            break;
        }
    }

    if(number<powerOf10) {
        powerOf10/=10;
    }

    while(number || powerOf10>0) {
        char digit='0';
        while(number>=powerOf10) {
            ++digit;
            number-=powerOf10;
        }
        output[idx++]=digit;
        if(idx>=buffersize) {
            // Cause a compile error
            throw "Internal text buffer overflow - increase internal buffers";
        }
        powerOf10/=10;
    }
    return idx;
}


_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ size_t outputDigits(int64_t number, int64_t exponent, char *output, size_t buffersize) {
    size_t idx=0;

    idx+= outputDigitsNoExponent(number,output,buffersize);

    if(exponent>0) {
        if(idx+exponent>16) {
        // Use scientific notation
        output[idx++]='e';
        idx+=outputDigitsNoExponent(exponent,output+idx,buffersize-idx);
        }
        else {
        // Just add zome zeroes to the output
            while(exponent>0) {
            output[idx++]='0';
                if(idx>=buffersize) {
                    // Cause a compile error
                    throw "Internal text buffer overflow - increase internal buffers";
                }
                exponent--;
            }
        }
    }
    return idx;
}

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ int64_t intabs(int64_t number) {
    return (number < 0) ? -number : number;
}

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ double powerOf10Exponent(const int64_t exp10) {
    if (!exp10) {
        return 1.0;
    }
    const auto absexp = intabs(exp10);
    if (absexp < 18) {
        int64_t ipowerOf10 = 1;
        // Should use a table for this, but for now...
        for (auto k = 0; k < absexp; ++k) {
            ipowerOf10 *= 10;
        }
        if (exp10 < 0) {
            return 1.0 / ipowerOf10;
        } else {
            return (double)ipowerOf10;
        }
    } else {
        return intpow(10.0, exp10);
    }
}

// Added this because MSVC does not support __int128 yet (when???)
struct integer128 {
    // 128 bit unsigned integers
    uint64_t loword;
    uint64_t hiword;

    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128(int64_t a)
        : loword(a), hiword(a < 0 ? -1 : 0) {}
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128(uint64_t lo, uint64_t hi)
        : loword(lo), hiword(hi) {}

    _OPTIMIZE_ static _ALWAYS_CONSTEXPR_ integer128 add128(integer128 a,
                                                           integer128 b) {
        // Shift the low word to capture the carry bit
        auto lw = (a.loword >> 1) + (b.loword >> 1) + (a.loword & b.loword & 1LL);
        // Add the high word with carry
        auto hw = a.hiword + b.hiword + ((lw >> 63) & 1);
        // Shift back the low word and insert the low bit back
        lw <<= 1;
        lw |= (b.loword ^ a.loword) & 1;
        return {lw, hw};
    }

    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 inc128(integer128 a) {
        if (a.loword == ~0ULL) {
            return {a.loword + 1, a.hiword + 1};
        }
        return {a.loword + 1, a.hiword};
    }

    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 neg128(integer128 a) {
        auto lw = ~a.loword;
        auto hw = ~a.hiword;
        return inc128({lw, hw});
    }
    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 abs128(integer128 a) {
        return (a.hiword & (1ULL << 63)) ? neg128(a) : a;
    }

    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 mul64x64(int64_t a,
                                                                    int64_t b) {
        auto sign = (b ^ a) >> 63;
        if (a < 0)
            a = -a;
        if (b < 0)
            b = -b;
        const uint64_t ah = ((uint64_t)a) >> 32;
        const uint64_t al = ((uint64_t)a) & 0xffffffff;
        const uint64_t bh = ((uint64_t)b) >> 32;
        const uint64_t bl = ((uint64_t)b) & 0xffffffff;

        // midword is guaranteed not to have carry because
        // it multiplies 32 bits x 31 bits = 63 bits
        // then adds two 63-bit numbers together = 64 bits, no carry
        auto midword = ah * bl + bh * al;
        const auto result =
            add128({al * bl, ah * bh}, {(midword << 32), (midword >> 32)});
        if (sign) {
            return neg128(result);
        }
        return result;
    }

    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ bool iszero128(integer128 a) {
        return (a.loword | a.hiword) == 0;
    }

    // LSL
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128 operator<<(int shift) {
        if (shift >= 64) {
            return {0ULL, loword << (shift - 64)};
        }
        if (shift <= 0) {
            return *this;
        }
        uint64_t hw = (hiword << shift) | (loword >> (64 - shift));
        uint64_t lw = loword << shift;
        return {lw, hw};
    }

    // LSR (unsigned, not ASR)
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128 operator>>(int shift) {
        if (shift >= 64) {
            return {hiword >> (shift - 64), 0ULL};
        }
        if (shift <= 0) {
            return *this;
        }
        uint64_t lw = (loword >> shift) | (hiword << (64 - shift));
        uint64_t hw = hiword >> shift;
        return {lw, hw};
    }

    _OPTIMIZE_ inline static _ALWAYS_CONSTEXPR_ uint64_t testbit128(integer128 a,
                                                                    int bit) {
        if (bit >= 128)
            return 0;
        if (bit < 0)
            return 0;
        if (bit >= 64)
            return (a.hiword & (1ULL << (bit - 64))) ? 1 : 0;
        return (a.loword & (1ULL << bit)) ? 1 : 0;
    }

    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ bool operator>=(integer128 b) {
        if (hiword == b.hiword)
            return loword >= b.loword;
        return hiword > b.hiword;
    }
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ bool operator<(integer128 b) {
        if (hiword == b.hiword)
            return loword < b.loword;
        return hiword < b.hiword;
    }

    // Division is flooring division, so the remainder has the same sign as the
    // dividend For example, -10/3 = (q==-3)*3+(r==-1)
    //              -10/-3 = (q==3)*(-3)+(r==-1)
    //              10/3 = (q==3)*3+(r==1)
    //              10/-3 = (q==-3)*(-3)+(r==1)
    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 div128(integer128 a,
                                                                  integer128 b) {
        integer128 quot{0};
        int shift = 0;
        int isneg = 0;
        if (iszero128(b)) {
            throw "Divide by zero";
        }
        if (iszero128(a)) {
            return a;
        }
        if (testbit128(a, 127)) {
            a = neg128(a);
            isneg ^= 1;
        }
        if (testbit128(b, 127)) {
            b = neg128(b);
            isneg ^= 1;
        }

        while (b < a) {
            b = b << 1;
            ++shift;
        }
        while (!iszero128(a) && shift >= 0) {
            quot = quot << 1;
            if (a >= b) {
                quot.loword |= 1;
                a = add128(a, neg128(b));
            }
            a = a << 1;
            --shift;
        }
        if (shift >= 0) {
            quot = quot << (shift + 1);
        }
        return isneg ? neg128(quot) : quot;
    }

    // Remainder operator: Uses flooring division, so the remainder has the same
    // sign as the dividend For example, -10/3 = (q==-3)*3+(r==-1)
    //              -10/-3 = (q==3)*(-3)+(r==-1)
    //              10/3 = (q==3)*3+(r==1)
    //              10/-3 = (q==-3)*(-3)+(r==1)

    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 rem128(integer128 a,
                                                                  integer128 b) {
        int shift = 0;
        if (iszero128(b) || iszero128(a)) {
            return a;
        }
        int isnegA = a.hiword >> 63;
        int isnegB = b.hiword >> 63;
        if (isnegA) {
            a = neg128(a);
        }
        if (isnegB) {
            b = neg128(b);
        }
        while (b < a) {
            b = b << 1;
            ++shift;
        }
        int initial_shift = shift;
        while (!iszero128(a) && shift >= 0) {
            if (a >= b) {
                a = add128(a, neg128(b));
            }
            a = a << 1;
            --shift;
        }
        a = a >> (initial_shift + 1);
        return (isnegA) ? neg128(a) : a;
    }
    // MODULO OPERATOR: Remainder but always in the range of [0..N-1] with
    // N==divisor
    _OPTIMIZE_ static inline _ALWAYS_CONSTEXPR_ integer128 mod128(integer128 a,
                                                                  integer128 b) {
        integer128 remainder = rem128(a, b);
        auto isnegA = a.hiword >> 63;
        auto isnegB = b.hiword >> 63;
        if (isnegA == isnegB) {
            return remainder;
        }
        return add128(remainder, b);
    }

    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128 operator%(integer128 b) {
        return rem128(*this, b);
    }

    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128 operator/(integer128 b) {
        return div128(*this, b);
    }
};

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ integer128 gcd128(integer128 a,
                                                       integer128 b) {
    while (!integer128::iszero128(b)) {
        auto t = b;
        b = a % b;
        a = t;
    }
    return a;
}

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ std::pair<int64_t, int64_t>
addfraction(int64_t a_num, int64_t a_den, int64_t b_num, int64_t b_den) {
    integer128 numerator = integer128::add128(integer128::mul64x64(a_num, b_den),
                                              integer128::mul64x64(b_num, a_den));
    integer128 denominator = integer128::mul64x64(a_den, b_den);
    auto divisor =
        gcd128(integer128::abs128(numerator), integer128::abs128(denominator));

    numerator = numerator / divisor;
    denominator = denominator / divisor;
    if ((numerator.hiword != 0ULL && numerator.hiword != ~0ULL) ||
        (denominator.hiword != 0ULL && denominator.hiword != ~0ULL)) {
        throw "Precision loss";
    }
    return {numerator.loword, denominator.loword};
}

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ std::pair<int64_t, int64_t>
simplifyfraction(int64_t a_num, int64_t a_den) {
    auto divisor = gcd(intabs(a_num), intabs(a_den));
    if (divisor > 1) {
        return {a_num / divisor, a_den / divisor};
    }
    return {a_num, a_den};
}

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ std::pair<int64_t, int64_t>
simplifyfraction128(integer128 a_num, integer128 a_den) {
    auto divisor = gcd128(integer128::abs128(a_num), integer128::abs128(a_den));
    if (divisor.hiword>0 || divisor.loword > 1) {
    a_num = a_num / divisor;
    a_den = a_den / divisor;
    }
    if ((a_num.hiword != 0ULL && a_num.hiword != ~0ULL) ||
        (a_den.hiword != 0ULL && a_den.hiword != ~0ULL)) {
        throw "Precision loss";
    }
    return {a_num.loword, a_den.loword};
}

// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

// Main class containing a unit definition
struct UnitDefinition {

    struct tokendata {
        size_t tokStart = 0;
        size_t tokEnd = 0;
        int64_t expNum = 0;
        int64_t expDen = 0;
    };

    // Default constructor creates the non-dimensional number 1.0
    _OPTIMIZE_ inline constexpr UnitDefinition()
        : u_name(), u_def(), value_ip(1), value_den(1), value_exp(0),
        error_state(NoError), error_index(0), definition({}) {}
    // Constructor using a single string literal, creates a unit definition with
    // no name
    template <size_t M>
    _OPTIMIZE_ inline constexpr UnitDefinition(const UTxt<M> U)
        : UnitDefinition("", U.value) {}
    // Main constructor and parser, creates a unit definition from a name and a
    // string
    template <size_t N, size_t M>
    _OPTIMIZE_ inline constexpr UnitDefinition(const char (&name)[N],
                                               const char (&defstring)[M]) {
        static_assert(N < maxTokenLength);
        static_assert(M < maxDefinitionLength);
        std::copy_n(name, N, u_name);
        std::copy_n(defstring, M, u_def);
        u_defLen = M;
        parseUnit();
    }

#ifdef RUNTIME_COMPONENT

    _OPTIMIZE_ inline constexpr UnitDefinition(const char *defstring,
                                               size_t len) {
        u_name[0] = 0;
        std::copy_n(defstring, len, u_def);
        u_defLen = len;
        parseUnit();
    }

#endif
    // Main parser of a unit definition. Extracts unit data from a definition
    // string previously stored in member u_def, with length u_defLen. Fills out
    // all other members.
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ void parseUnit() {
        bool haveDen = false;
        // Numerical part or a unit is composed of 3 64-bit integers:
        // (numerator/denominator)*10^exponent This provides same of better range
        // than any double precision, while using integer arithmetic during compile
        // time
        int64_t intPart = 0;
        int64_t expTen = 0;
        int64_t denPart = 0;
        int64_t denExpTen = 0;
        int64_t expDigits = 0;
        bool haveExponent = false;
        bool expNeg = false;
        // Parse the value first
        int expmove = 0;
        size_t i = 0;
        while (i < u_defLen) {
            if (u_def[i] >= '0' && u_def[i] <= '9') {
                if (haveExponent) {
                    expDigits *= 10;
                    expDigits += u_def[i] - '0';
                } else {
                    intPart *= 10;
                    intPart += u_def[i] - '0';
                    expTen += expmove;
                }
            } else {
                if (!haveExponent && u_def[i] == '.') {
                    expmove = 1;
                } else {
                    if (!haveExponent && u_def[i] == '/' && !haveDen &&
                        u_def[i + 1] >= '0' && u_def[i + 1] <= '9') {
                        // Looks like we have a denominator in the value
                        // Store the current number in the denominator and start over, we'll
                        // swap them later
                        denPart = intPart;
                        denExpTen = expTen + (expNeg ? -expDigits : expDigits);
                        intPart = 0;
                        expTen = 0;
                        expmove = 0;
                        expNeg = false;
                        haveExponent = false;
                        haveDen = true;
                    } else {
                        if (!haveExponent && (u_def[i] == 'e' || u_def[i] == 'E') &&
                            ((u_def[i + 1] >= '0' && u_def[i + 1] <= '9') ||
                                                                                      u_def[i + 1] == '-')) {
                            // Valid exponent, switch to exponent mode
                            haveExponent = true;
                            expDigits = 0;
                            expNeg = false;
                        } else {
                            if (haveExponent && !expNeg && expDigits == 0 &&
                                u_def[i] == '-') {
                                expNeg = true;
                            } else {
                                if (u_def[i] == '\'') {
                                    // Do nothing, but accept it as a separator
                                } else {
                                    // No other chars allowed in the number
                                    // In the future, see if we can also accept
                                    // exponents here
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            ++i;
        }
        if (haveExponent && expDigits == 0) {
            // Do not consume the last character, presumably the 'e' or a '-'
            // either way it should trigger a bad token
            if (i > 0) {
                --i;
            }
        }
        if (haveDen) {
            // Swap numerator and denominator
            auto temp = intPart;
            intPart = denPart;
            denPart = temp;
            // Subtract the exponents of numerator and denominator, we use only one
            // exponent
            expTen = denExpTen - expTen + (expNeg ? -expDigits : expDigits);
        } else {
            denPart = 1;
            expTen -= expNeg ? -expDigits : expDigits;
        }
        // If no value is provided, assume 1.0
        if (intPart == 0) {
            intPart = 1;
            expTen = 0;
        }

        // Done with the value, get the sequence of units
        // by consuming tokens
        struct parenthesisdata {
            int firstToken = 0;
            int lastToken = 0;
            int64_t numExp = 0;
            int64_t denExp = 0;
        };

        std::array<tokendata, maxTokens> allTokens;
        std::array<parenthesisdata, 50> parenLevel;
        int nTokens = 0;
        int nParen = 0;

        if (u_def[i] == '_') {
            ++i;
        }
        bool lastTokenWasParen = false;
        bool lastTokenWasOperator = false;
        size_t tokStart = i;
        size_t tokEnd = i;
        int64_t expnum = 1;
        int64_t expden = 1;

        // Lambda function to consume a parenthesis level
        auto consumeParenthesis = [&]() {
            // Consume the whole parenthesis
            if(nParen>0) {
            for (auto j = parenLevel[nParen - 1].firstToken;
                 j < parenLevel[nParen - 1].lastToken; ++j) {
                // Apply the current exponent to all the items in the parenthesis
                allTokens[j].expNum *= parenLevel[nParen - 1].numExp;
                allTokens[j].expDen *= parenLevel[nParen - 1].denExp;
            }
            --nParen;
            lastTokenWasParen = false;
            return true;
            }
            // Bad Parenthesis, do not consume it to cause an error
            return false;
        };

        // Lambda function to consume an exponent in fractional form
        auto consumeNumericExponent = [&]() {

            bool haveNum = false;
            bool needDen = false;
            bool haveSign = false;
            bool numberInProgress = false;
            bool numberEnded = false;
            int64_t num = 0;
            int64_t den = 0;

            bool isNeg = i < (u_defLen - 1) && u_def[i] == '-';
            if (isNeg) {
                haveSign = true;
                ++i;
            }
            bool haveParen = i < (u_defLen - 1) && u_def[i] == '(';
            if (haveParen) {
                ++i;
            }
            while (i < (u_defLen - 1)) {
                if (u_def[i] >= '0' && u_def[i] <= '9') {
                    if (numberEnded)
                        break;
                    haveSign = true;
                    if (!haveNum) {
                        if (num > 922337203685477579LL) {
                            // Integer overflow
                            den = -1;
                            break;
                        }
                        num *= 10;
                        num += u_def[i] - '0';
                        numberInProgress = true;
                    } else {
                        den *= 10;
                        if (den > 922337203685477579LL) {
                            // Integer overflow
                            den = -1;
                            break;
                        }
                        den += u_def[i] - '0';
                        needDen = false;
                        numberInProgress = true;
                    }
                } else {
                    if (u_def[i] == '/' && !haveNum) {
                        if (!numberInProgress && !numberEnded) {
                            break;
                        }
                        haveNum = true;
                        needDen = true;
                        haveSign = false;
                        numberInProgress = false;
                        numberEnded = false;
                    } else {
                        if (u_def[i] == ')' && haveParen &&
                            (numberInProgress || numberEnded)) {
                            ++i;
                            haveParen = false;
                            break;
                        } else {
                            if (u_def[i] == '-' && !haveSign && !numberEnded &&
                                !numberInProgress) {
                                haveSign = true;
                                isNeg = !isNeg;
                            } else {
                                if (u_def[i] == ' ') {
                                    if (numberInProgress) {
                                        numberInProgress = false;
                                        numberEnded = true;
                                    }
                                } else {
                                    // No other chars allowed in
                                    // the number In the future,
                                    // see if we can also accept
                                    // exponents here
                                    break;
                                }
                            }
                        }
                    }
                }
                ++i;
            }

            if (den == 0) {
                den = 1;
            }
            if (isNeg) {
                num = -num;
            }
            // If the previous symbol was a division and there was no
            // parenthesis, don't consume the operator since it is
            // operating on the unit, not the exponent
            if (!haveParen && needDen) {
                // We shouldn't consume the last operator
                while(i>0 && (u_def[i-1]==' ' || u_def[i-1]=='/')) {
                    --i;
                }
            } else {
                // If the exponent had a parenthesis or was missing a
                // denominator, fail with an error
                if (haveParen || needDen || (den < 0)) {
                    // Syntax error in the exponent
                    den = 0;
                }
            }
            // Apply the exponent to the last token or the last
            // parenthesis group
            if (lastTokenWasParen) {
                // Apply the exponent to the whole parenthesis instead of the current token
                if(nParen>0) {
                    parenLevel[nParen - 1].numExp*=num;
                    parenLevel[nParen - 1].denExp*=den;
                }
            } else {
                // Just the last token
                allTokens[nTokens - 1].expNum *= num;
                allTokens[nTokens - 1].expDen *= den;
            }
            tokStart = tokEnd = i;
            // Reset next exponent
            expnum = 1;
            expden = 1;
        };

        // End of Lambda functions

        // Main Loop, start consuming characters
        while (i < (u_defLen - 1)) {
            if (u_def[i] == '*' || u_def[i] == '-') {
                // This is a multiplication
                if (lastTokenWasParen) {
                    if(!consumeParenthesis()) {
                        break;
                    }
                } else {
                    if (i > tokStart) {
                        tokEnd = i;
                        // Consume the token
                        allTokens[nTokens] = {tokStart, tokEnd, expnum, expden};
                        ++nTokens;
                    }
                }
                tokStart = tokEnd = i + 1;
                if(lastTokenWasOperator) {
                    // Can't have operators back-to-back
                    break;
                }
                // Reset next exponent
                expnum = 1;
                expden = 1;
                lastTokenWasOperator = true;
            } else {
                if (u_def[i] == ' ') {
                    // This is a multiplication unless it's the beginning of a token
                    if (lastTokenWasParen) {
                        if(!consumeParenthesis()) {
                            break;
                        }
                    } else {
                        if (i > tokStart) {
                            tokEnd = i;
                            // Consume the token
                            allTokens[nTokens] = {tokStart, tokEnd, expnum, expden};
                            ++nTokens;
                        }
                    }
                    tokStart = tokEnd = i + 1;
                    if(!lastTokenWasOperator) {
                    // The space becomes a multiplication
                    // Reset next exponent
                    expnum = 1;
                    expden = 1;
                    }
                } else {
                    if (u_def[i] == '/') {
                        if (lastTokenWasParen) {
                            if(!consumeParenthesis()) {
                                break;
                            }
                        } else {
                            if (i > tokStart) {
                                tokEnd = i;
                                // Consume the token
                                allTokens[nTokens] = {tokStart, tokEnd, expnum, expden};
                                ++nTokens;
                            }
                            tokStart = tokEnd = i + 1;
                        }
                        if(lastTokenWasOperator) {
                            break;
                        }
                        // Reset next exponent
                        expnum = -1;
                        expden = 1;
                        lastTokenWasOperator = true;
                    } else {
                        if (u_def[i] == '(') {
                            // Opening a parenthesis in a token implies multiplication
                            if(lastTokenWasParen) {
                            if(!consumeParenthesis()) {
                                break;
                            }
                            }
                            if (i > tokStart) {
                                // We have an open token, close it and multiply
                                tokEnd = i;
                                // Consume the token
                                allTokens[nTokens] = {tokStart, tokEnd, expnum, expden};
                                ++nTokens;
                            }
                            tokStart = tokEnd = i + 1;
                            parenLevel[nParen].firstToken = nTokens;
                            parenLevel[nParen].lastToken = nTokens;
                            parenLevel[nParen].numExp = expnum;
                            parenLevel[nParen].denExp = expden;
                            ++nParen;
                            expnum = 1;
                            expden = 1;
                            lastTokenWasOperator = false;
                        } else {
                            if (u_def[i] == ')') {
                                if(lastTokenWasParen) {
                                    if(!consumeParenthesis()) {
                                        break;
                                    }
                                } else {
                                    if (i > tokStart) {
                                    // We have an open token, close it and
                                        // multiply
                                    tokEnd = i;
                                    // Consume the token
                                    allTokens[nTokens] = {tokStart, tokEnd,
                                                          expnum, expden};
                                    ++nTokens;
                                    }
                                }
                                tokStart = tokEnd = i + 1;
                                // Reset next exponent
                                expnum = 1;
                                expden = 1;
                                if (nParen > 0) {
                                    // Close the parenthesis but don't consume it yet, there might be an exponent for the whole parenthesis coming
                                    parenLevel[nParen - 1].lastToken = nTokens;
                                }
                                   lastTokenWasParen = true;
                                   lastTokenWasOperator = false;
                            } else {
                                if (u_def[i] == '^') {
                                    // Consume a numerical exponent in fractional
                                    // form, accept parenthesis
                                    if (i > tokStart) {
                                        // We have an open token, close it and
                                        // multiply
                                        tokEnd = i;
                                        // Consume the token
                                        allTokens[nTokens] = {tokStart, tokEnd,
                                                              expnum, expden};
                                        ++nTokens;
                                    }
                                    tokStart = tokEnd = i + 1;

                                    if(lastTokenWasOperator) {
                                        break;
                                    }

                                    ++i;
                                    consumeNumericExponent();
                                    // If the exponent was applied to a parenthesis, consume the closed parenthesis
                                    if(lastTokenWasParen) {
                                        if(!consumeParenthesis()) {
                                            break;
                                        }
                                    }
                                    continue;   // We've already consumed the current character, no need to increment it again
                                }
                                else {
                                    // Any other character is part of a token, consume it
                                    lastTokenWasOperator = false;
                                }
                            }
                        }
                    }
                }
            }
            ++i;
        }
        if (lastTokenWasParen) {
            // Apply the current exponent to the whole parenthesis instead of the current token
            if(nParen>0) {
                parenLevel[nParen - 1].numExp*=expnum;
                parenLevel[nParen - 1].denExp*=expden;
            }
            consumeParenthesis();
        } else {
            // Did we have an open token?
            if (i > tokStart && tokStart != u_defLen - 1) {
                tokEnd = i;
                if (i == u_defLen)
                    tokEnd--;
                // Consume the token
                allTokens[nTokens] = {tokStart, tokEnd, expnum, expden};
                ++nTokens;
                tokStart = tokEnd = i + 1;
            }
        }

        // Update Initial Error State
        error_state = UnitError::NoError;
        error_index = 0;
        if (nParen != 0) {
            error_state = UnitError::BadParenthesis;
            error_index = u_defLen - 1;
        }
        if(lastTokenWasOperator) {
            error_state = UnitError::InvalidDefinition;
            error_index = i;
        }

        for (auto i = 0; i < nTokens; ++i) {
            if (allTokens[i].tokStart == allTokens[i].tokEnd) {
                error_state = UnitError::InvalidDefinition;
                error_index = allTokens[i].tokStart;
                break;
            } else {
                for (auto j = allTokens[i].tokStart; j < allTokens[i].tokEnd; ++j) {
                    // No numbers in a token, otherwise it's fair game to use Unicode
                    // chars (Angstrom, Micron, etc.)
                    if (u_def[j] >= '0' && u_def[j] <= '9') {
                        error_state = UnitError::InvalidDefinition;
                        error_index = allTokens[i].tokStart + j;
                        break;
                    }
                }
            }
            if (allTokens[i].expDen == 0) {
                error_state = UnitError::InvalidDefinition;
                error_index = allTokens[i].tokEnd;
                break;
            }
        }

        // Simplify the fraction
        auto simpfrac = simplifyfraction(intPart, denPart);
        // Base 10 exponent correction
        while (simpfrac.first % 10 == 0) {
            simpfrac.first /= 10;
            expTen--;
        }
        while (simpfrac.second % 10 == 0) {
            simpfrac.second /= 10;
            expTen++;
        }

        // Store the final number - no loss of precision
        value_ip = simpfrac.first;
        value_den = simpfrac.second;
        value_exp = -expTen;
        definition = allTokens;
    }

    // Min operations with units, all create new UnitDefinition objects and are
    // strictly consteval

    // Inverse of a unit U^(-1)
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition invert() const {
        UnitDefinition result = *this;
        result.value_ip = value_den;
        result.value_den = value_ip;
        result.value_exp = -value_exp;
        for (size_t i = 0; i < maxTokens; ++i) {
            result.definition[i].expNum = -result.definition[i].expNum;
            if (definition[i].tokStart == definition[i].tokEnd)
                break;
        }
        return result;
    }

    // Divide two different units
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition
    divide(const UnitDefinition &other) const {
        UnitDefinition inverse = other.invert();
        return multiply(inverse);
    }

    // Multiply two different units
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition
    multiply(const UnitDefinition &other) const {
        UnitDefinition result = *this;

        // We'll need to add to the definition string to keep the token strings,
        // find the end
        size_t location;
        for (location = 0; u_def[location] != 0 && location < maxDefinitionLength;
             ++location)
            ;
        // We'll need to add tokens at the end of the definition, find the end
        size_t ntokens;
        for (ntokens = 0;
             definition[ntokens].tokEnd > definition[ntokens].tokStart &&
             ntokens < maxTokens;
             ++ntokens)
            ;

        for (size_t i = 0; i < maxTokens; ++i) {
            auto otherlen = other.definition[i].tokEnd - other.definition[i].tokStart;
            if (otherlen == 0)
                break;
            size_t j;
            size_t len;
            for (j = 0; j < maxTokens; ++j) {
                len = definition[j].tokEnd - definition[j].tokStart;
                if (len == 0) {
                    break;
                }
                if (len != otherlen) {
                    continue;
                }
                size_t k;
                for (k = 0; k < len; ++k) {
                    if (u_def[definition[j].tokStart + k] !=
                        other.u_def[other.definition[i].tokStart + k]) {
                        break;
                    }
                }
                if (k == len) {
                    // Found the symbol, search no more
                    break;
                }
            }
            if (j != maxTokens && len > 0) {
                // Found the symbol, so add the exponents
                auto numDenPair =
                    addfraction(definition[j].expNum, definition[j].expDen,
                                              other.definition[i].expNum, other.definition[i].expDen);
                result.definition[j].expNum = numDenPair.first;
                result.definition[j].expDen = numDenPair.second;
            } else {
                // This symbol is not on the definition, will need to add it
                if (location + otherlen >= maxDefinitionLength) {
                    result.error_index = 0;
                    result.error_state = UnitError::DefinitionTooLong;
                    return result;
                }
                for (size_t k = 0; k < otherlen; ++k) {
                    result.u_def[location + k] =
                        other.u_def[other.definition[i].tokStart + k];
                }
                auto newTokenStart = location;
                location += otherlen;
                auto newTokenEnd = location;
                result.u_def[location] = 0;

                if (ntokens + 1 > maxTokens) {
                    result.error_index = 0;
                    result.error_state = UnitError::TooManyTokens;
                    return result;
                }

                result.definition[ntokens].expNum = other.definition[i].expNum;
                result.definition[ntokens].expDen = other.definition[i].expDen;
                result.definition[ntokens].tokStart = newTokenStart;
                result.definition[ntokens].tokEnd = newTokenEnd;
                ++ntokens;
            }
        }
        // All tokens were processed, eliminate any tokens that have zero exponent
        size_t src, dest;
        for (src = 0, dest = 0; src < maxTokens; ++src) {
            if (result.definition[src].expNum == 0) {
                continue;
            }
            if (src != dest) {
                result.definition[dest] = result.definition[src];
            }
            ++dest;
        }
        result.definition[dest] = {0, 0, 0, 0};

        // Multiply the values of the two
        // Watch out for integer overflow in these multiplications
        result.value_ip *= other.value_ip;
        result.value_den *= other.value_den;
        result.value_exp += other.value_exp;

        // definition string needs to be regenerated
        //        result.regeneratestring();
        return result;
    }

    // Apply a fractional exponent to a unit
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition
    pow(int64_t expNum, int64_t expDen) const {
        UnitDefinition result = *this;
        if(expDen<0) {
            expNum=-expNum;
            expDen=-expDen;
        }
        if (expDen == 1) {
            // TODO: Watch for integer overflow here! Even though unit exponents tend
            // to be small numbers, the integer constant can be a big number
            if (expNum < 0) {
                result.value_den = intpow(value_ip, -expNum);
                result.value_exp = expNum * value_exp;
                result.value_ip = intpow(value_den, -expNum);
            } else {
                result.value_ip = intpow(value_ip, expNum);
                result.value_exp *= expNum;
                result.value_den = intpow(value_den, expNum);
            }
        } else {
            if (result.value_ip != 1 || result.value_den != 1 ||
                result.value_exp != 0) {
                // Fractional powers, use floationg point then come back to integers
                // GCC has std::pow constexpr as a non-standard extension, but clang
                // will refuse this because pow is NOT constexpr
                bool invertFraction = expNum<0;
                double numerator = intpow(introot((double)result.value_ip,expDen),intabs(expNum));
                double denominator = intpow(introot((double)result.value_den,expDen),intabs(expNum));

                // Now try to extract a fraction
                int num_exponent2 = dbl_exponent(numerator);
                double num_mant = dbl_mantissa(numerator);
                double int_num = dbl_make(num_mant, 54);
                int den_exponent2 = dbl_exponent(denominator);
                double den_mant = dbl_mantissa(denominator);
                double int_den = dbl_make(den_mant, 54);

                integer128 num128{static_cast<int64_t>(
                    int_num)}; // Multiply by 2^64 to make it an integer, this will
                // only change the IEEE exponent
                integer128 den128{static_cast<int64_t>(int_den)};

                num_exponent2 -= den_exponent2;

                if (num_exponent2 > 64 || num_exponent2 < -64) {
                    throw "Precision loss in pow operation, cannot proceed";
                }
                if (num_exponent2 < 0) {
                    den128 = den128 << (-num_exponent2);
                }
                if (num_exponent2 > 0) {
                    num128 = num128 << num_exponent2;
                }

                auto fraction = simplifyfraction128(num128, den128);

                if(invertFraction) {
                    result.value_den=fraction.first;
                    result.value_ip=fraction.second;
                }
                else {
                result.value_ip = fraction.first;
                result.value_den = fraction.second;
                }
            }
        }

        for (size_t i = 0; i < maxTokens; ++i) {
            auto fractionexponent = simplifyfraction(result.definition[i].expNum * expNum, result.definition[i].expDen * expDen);
            result.definition[i].expNum = fractionexponent.first;
            result.definition[i].expDen = fractionexponent.second;
            if (definition[i].tokStart == definition[i].tokEnd)
                break;
        }
        return result;
    }

    // Simplify a unit by searching through all unit definitions and recursively
    // replacing each named unit with its definition until no more replacements
    // are possible. This produces an equivalent complex unit where all units were
    // reduced to base units
    // Defined outside of the class in order to have the entire unit definition
    // list
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition simplify() const;

    // Creates a unit with a single unit token and exponent, taken from the
    // current object
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition
    extractToken(size_t index) const {
        UnitDefinition result;
        result.definition[0] = definition[index];
        size_t tokenLen = definition[index].tokEnd - definition[index].tokStart;
        for (size_t i = 0; i < tokenLen; ++i) {
            result.u_def[i] = u_def[definition[index].tokStart + i];
        }
        result.u_def[tokenLen] = 0;
        result.definition[0].tokStart = 0;
        result.definition[0].tokEnd = tokenLen;
        return result;
    }

    // Update the text for the unit definition after a unit was operated upon
    // Uses the list of tokens and exponents to recreate the string.
    _OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition update() const {
        // Rebuild the u_def string based off the token list and exponents
        // TODO
        UnitDefinition result;
        result.value_ip = value_ip;
        result.value_den = value_den;
        result.value_exp = value_exp;
        result.error_state = error_state;
        result.error_index = 0; // Changing the u_def member will reset this to zero

        size_t txtj = 0;

        // If the number is anything but one, output the number first
        if(value_ip!=1 || value_den!=1 || value_exp!=0) {
            const bool needDen = (value_den!=1)||(value_exp<0);

            // Output numerator first
            txtj += outputDigits(value_ip,(value_exp>0)? value_exp: 0, result.u_def+txtj, maxDefinitionLength-txtj);
            if(needDen) {
                result.u_def[txtj++]='/';
                txtj += outputDigits(value_den,((value_exp<0)? -value_exp: 0), result.u_def+txtj, maxDefinitionLength-txtj);
            }
        }

        bool isFirstToken=true;
        // Need a count of tokens with negative exponent to group them later if needed
        size_t numberOfTokensWithNegativeExp = 0;
        size_t tokensWithPosExpStart = txtj;

        size_t j = 0;

        // Add the tokens with positive exponents first
        for (size_t i = 0; i < maxTokens; ++i) {
            if (definition[i].tokEnd == definition[i].tokStart) {
                break;
            }
            if(definition[i].expNum<0) {
                ++numberOfTokensWithNegativeExp;
            }
            if (definition[i].expNum > 0) {
                // Add a prefix if needed
                if(isFirstToken) {
                    if(txtj>0) {
                    result.u_def[txtj++]='_';
                    if(txtj>=maxDefinitionLength) {
                        throw "Internal Buffer overflow - Increase buffer size";
                    }
                    }
                    tokensWithPosExpStart = txtj;
                    isFirstToken=false;
                }
                // Copy the token to the result
                result.definition[j].expNum = definition[i].expNum;
                result.definition[j].expDen = definition[i].expDen;
                result.definition[j].tokStart = txtj;
                result.definition[j].tokEnd =
                    txtj + definition[i].tokEnd - definition[i].tokStart;
                ++j;
                // Add the token as a string
                if (txtj > tokensWithPosExpStart) {
                    result.u_def[txtj++] = '*';
                    if(txtj>=maxDefinitionLength) {
                        throw "Internal Buffer overflow - Increase buffer size";
                    }
                }
                for (auto k = definition[i].tokStart; k < definition[i].tokEnd; ++k) {
                    result.u_def[txtj] = u_def[k];
                    ++txtj;
                    if(txtj>=maxDefinitionLength) {
                        throw "Internal Buffer overflow - Increase buffer size";
                    }
                }
                // Add the exponent to the string
                if (definition[i].expNum != 1 || definition[i].expDen != 1) {
                    // Need to add an exponent
                    result.u_def[txtj++] = '^';
                    //
                    txtj += outputDigitsNoExponent(definition[i].expNum, result.u_def+txtj, maxDefinitionLength-txtj);

                    if (definition[i].expDen != 1) {
                        result.u_def[txtj++] = '/';
                        txtj += outputDigitsNoExponent(definition[i].expDen, result.u_def+txtj, maxDefinitionLength-txtj);
                    }
                }
            }
        }
        // Second pass, add all of the tokens with negative exponents
        bool isFirstNegExpToken = true;
        size_t firstNegTokenStart = txtj;

        for (size_t i = 0; i < maxTokens; ++i) {
            if (definition[i].tokEnd == definition[i].tokStart) {
                break;
            }
            if (definition[i].expNum < 0) {
                if(isFirstToken) {
                    // If there was a number before, add an underscore
                    if(txtj>0) {
                        result.u_def[txtj++]='_';
                        if(txtj>=maxDefinitionLength) {
                            throw "Internal Buffer overflow - Increase buffer size";
                        }
                        if(numberOfTokensWithNegativeExp>0) {
                            result.u_def[txtj++]='1';
                            if(txtj>=maxDefinitionLength) {
                                throw "Internal Buffer overflow - Increase buffer size";
                            }
                        }

                    }
                    firstNegTokenStart = txtj;
                    isFirstToken = false;
                }
                if(isFirstNegExpToken) {
                    if(numberOfTokensWithNegativeExp>0) {
                        if(txtj==0) {
                            result.u_def[txtj++]='1';
                            if(txtj>=maxDefinitionLength) {
                                throw "Internal Buffer overflow - Increase buffer size";
                            }
                        }
                        result.u_def[txtj++]='/';
                        if(txtj>=maxDefinitionLength) {
                            throw "Internal Buffer overflow - Increase buffer size";
                        }
                    }
                    if(numberOfTokensWithNegativeExp>1) {
                        result.u_def[txtj++]='(';
                        if(txtj>=maxDefinitionLength) {
                            throw "Internal Buffer overflow - Increase buffer size";
                        }
                    }

                    firstNegTokenStart = txtj;
                    isFirstNegExpToken = false;
                }

                // Copy the token to the result
                result.definition[j].expNum = definition[i].expNum;
                result.definition[j].expDen = definition[i].expDen;
                // Add the token as a string
                if (txtj > firstNegTokenStart) {
                    result.u_def[txtj++] = '*';
                    if(txtj>=maxDefinitionLength) {
                        throw "Internal Buffer overflow - Increase buffer size";
                    }
                }
                result.definition[j].tokStart = txtj;
                result.definition[j].tokEnd =
                    txtj + definition[i].tokEnd - definition[i].tokStart;
                ++j;
                for (auto k = definition[i].tokStart; k < definition[i].tokEnd; ++k) {
                    result.u_def[txtj] = u_def[k];
                    ++txtj;
                }
                // Add the exponent to the string
                if (definition[i].expNum != -1 || definition[i].expDen != 1) {
                    // Need to add an exponent
                    result.u_def[txtj++] = '^';
                    //
                    txtj += outputDigitsNoExponent(-definition[i].expNum,result.u_def+txtj,maxDefinitionLength-txtj);

                    if (definition[i].expDen != 1) {
                        result.u_def[txtj++] = '/';
                        txtj += outputDigitsNoExponent(definition[i].expDen,result.u_def+txtj,maxDefinitionLength-txtj);

                    }
                }
            }
        }

        if(numberOfTokensWithNegativeExp>1) {
            // Close that parenthesis we added
            result.u_def[txtj++]=')';
        }
        result.u_def[txtj++] = 0;
        result.u_defLen = txtj;
        return result;
    }

    // Name of the unit in question (abbreviated form used in formulae, not a
    // formal name, like "Pa" for Pascals) Can be an empty string, an unnamed unit
    char u_name[maxTokenLength] = {};
    // Definition of the unit as text: use "1" for base units, or a proper
    // definition in terms of other units for derived units
    char u_def[maxDefinitionLength] = {};

    size_t u_defLen = 0;
    // The numerical value of the definition, expressed as a fraction and a
    // base-10 exponent: numerator/denominator * 10^exponent
    int64_t value_ip;
    int64_t value_den;
    int64_t value_exp;
    // Error tracking for run-time error checking or static asserts
    enum UnitError error_state;
    // Position within the definition string where the parser found the error
    size_t error_index;
    // List of tokens and exponents extracted from the definition
    // List ends with a token where start==end
    std::array<tokendata, maxTokens> definition;
};

// Now we include the definition of all units here
#include "allunits.hpp"

// This is the only member of UnitDefinition defined outside the class
// definition because it needs the list of all units to exist as a _CONSTEXPR_
// to replace the units with their definitions For example, if the definitions
// are:
// "?m"="1" (base unit for length, accepting SI prefixes)
// "in"="25.4_mm"
// then the expression "in^2/m^2" will be expanded to:
// "(25.4_mm)^2/m^2" the unit in is replaced with its definition,
// while the meter is not replaced because it's a base unit
// The next pass becomes "25.4^2*(1/1000_m)^2/m^2" as the "mm" unit gets
// replaced by extracting the SI prefix
// Finally, the exponents for the 'm' are added, and the whole token
// removed because the addition results in a zero exponent
// becoming "25.4^2/1000^2"

_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ UnitDefinition
UnitDefinition::simplify() const {
    UnitDefinition result = *this;

    bool expansionHappened;

    do {
        expansionHappened = false;
        UnitDefinition expanded;
        // Take only the value, we'll fill out the units later
        expanded.value_ip = result.value_ip;
        expanded.value_den = result.value_den;
        expanded.value_exp = result.value_exp;

        // Scan through all the units, expand any that exists in the table
        for (size_t i = 0; i < maxTokens; ++i) {
            size_t tokenLen =
                result.definition[i].tokEnd - result.definition[i].tokStart;
            if (!tokenLen)
                break;
            // See if a token exists in the table, search by name
            bool isAMatch = false;
            for (size_t j = 0; j < sizeof(allUnits) / sizeof(UnitDefinition) &&
                               allUnits[j].value_ip != 0;
                 ++j) {
                size_t namestart = allUnits[j].u_name[0] == '?' ? 1 : 0;
                isAMatch = true;
                // Compare the name
                if (allUnits[j].u_name[namestart + tokenLen] == 0) {
                    for (size_t k = 0;
                         k < maxTokenLength &&
                         result.definition[i].tokStart + k < result.definition[i].tokEnd;
                         ++k) {
                        if (result.u_def[result.definition[i].tokStart + k] !=
                            allUnits[j].u_name[namestart + k]) {
                            isAMatch = false;
                            break;
                        }
                    }
                } else {
                    isAMatch = false;
                }
                if (isAMatch) {
                    // Only replace if it's a derived unit AND it doesn't start with '#'
                    // # is used for numerical constants = non-dimensional units that are
                    // NOT base units and must be replaced immediately
                    if (allUnits[j].definition[0].tokStart ==
                            allUnits[j].definition[0].tokEnd &&
                        allUnits[j].u_name[0] != '#') {
                        // This is a base unit, do NOT replace
                        isAMatch = false;
                        break;
                    } else {
                        // Apply the exponent to the definition
                        UnitDefinition replacement = allUnits[j].pow(
                            result.definition[i].expNum, result.definition[i].expDen);
                        // And multiply it to our result
                        expanded = expanded.multiply(replacement);
                        expansionHappened = true;
                        break;
                    }
                } else {
                    // If this unit supports SI prefixes, we need to do a second
                    // comparison
                    if (namestart > 0) {
                        // TODO: Strip the SI prefix from the unit and compare again
                        size_t newtokenStart = result.definition[i].tokStart;
                        _CONSTEXPR_ char siPrefixes[] = "QRYZEPTGMkhdcmnpfazyrq";
                        int64_t siPrefixExponent = 0;
                        size_t si;
                        for (si = 0; siPrefixes[si] != 0; ++si) {
                            if (result.u_def[result.definition[i].tokStart] ==
                                siPrefixes[si]) {
                                break;
                            }
                        }
                        // If it was one of the letters, skip it
                        // Handle 2 special cases: deka and micro
                        if (siPrefixes[si] != 0) {
                            newtokenStart++;
                            if (si <= 9) {
                                siPrefixExponent = 30 - 3 * si;
                            }
                            if (si == 10) {
                                siPrefixExponent = 2;
                            }
                            if (si == 11) {
                                siPrefixExponent = -1;
                            }
                            if (si == 12) {
                                siPrefixExponent = -2;
                            }
                            if (si == 13) {
                                siPrefixExponent = -3;
                            }
                            if (si >= 14) {
                                siPrefixExponent = -9 - 3 * (si - 14);
                            }
                            if (siPrefixes[si] == 'd' &&
                                result.u_def[newtokenStart + 1] == 'a' && tokenLen > 2) {
                                ++newtokenStart;
                                siPrefixExponent = 1;
                            }
                        } else {
                            // For Micro, we need to look for the Unicode symbol in UTF-8
                            // It can be U+03bc=={0xce, 0xbc} or U+00B5=={0xc2,0xb5}
                            if (tokenLen > 2 &&
                                (unsigned char)(result.u_def[newtokenStart]) == 0xc2 &&
                                (unsigned char)(result.u_def[newtokenStart + 1]) == 0xb5) {
                                newtokenStart += 2;
                                siPrefixExponent = -6;
                            }
                            if (tokenLen > 2 &&
                                (unsigned char)result.u_def[newtokenStart] == 0xce &&
                                (unsigned char)result.u_def[newtokenStart + 1] == 0xbc) {
                                newtokenStart += 2;
                                siPrefixExponent = -6;
                            }
                        }

                        // Do the whole comparison once more
                        isAMatch = true;
                        size_t k;
                        for (k = 0; k < maxTokenLength &&
                                    newtokenStart + k < result.definition[i].tokEnd;
                             ++k) {
                            if (result.u_def[newtokenStart + k] !=
                                allUnits[j].u_name[namestart + k]) {
                                isAMatch = false;
                                break;
                            }
                            if (result.u_def[newtokenStart + k] == 0) {
                                break;
                            }
                        }
                        if (allUnits[j].u_name[namestart + k] != 0) {
                            isAMatch = false;
                        }
                        if (isAMatch) {
                            // Apply the exponent to the definition
                            UnitDefinition prefixedUnit = allUnits[j];
                            if (prefixedUnit.definition[0].tokStart ==
                                prefixedUnit.definition[0].tokEnd) {
                                // This is a base unit, with an SI prefix applied
                                // Replace with the base unit name, skipping the '?' prefix in
                                // the name
                                size_t t;
                                for (t = 0; prefixedUnit.u_name[t + 1] != 0; ++t) {
                                    prefixedUnit.u_def[t] = prefixedUnit.u_name[t + 1];
                                }
                                prefixedUnit.u_def[t] = 0;
                                prefixedUnit.definition[0] = {0, t, 1, 1};
                            }
                            prefixedUnit.value_exp += siPrefixExponent;
                            UnitDefinition replacement = prefixedUnit.pow(
                                result.definition[i].expNum, result.definition[i].expDen);
                            // And multiply it to our result
                            expanded = expanded.multiply(replacement);
                            expansionHappened = true;
                            break;
                        }
                    }
                }
            }
            if (!isAMatch) {
                // We need to add this token to the expansion, doesn't alter the value
                // Apply the exponent to the definition
                UnitDefinition replacement = result.extractToken(i);
                // And multiply it to our result
                expanded = expanded.multiply(replacement);
            }
        }
        result = expanded;
    } while (expansionHappened);

    return result;
}

template <size_t N>
_OPTIMIZE_ inline _ALWAYS_CONSTEXPR_ auto to_UTxt(const UnitDefinition def) {
    char result[N]{};
    std::copy_n(def.u_def, N, result);
    return UTxt<N>{result};
}

// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

#ifdef RUNTIME_COMPONENT
class RQty;
#endif

// This is the main 'Quantity' class, with a string as a template argument
// that represents the unit definition
template <UTxt U> class Qty {
public:
    // Default constructor creates the non-dimensional number 0.0
    _OPTIMIZE_ _ALWAYS_CONSTEXPR_ inline Qty() : number(0.0) {}
    // Constructor with an integer number
    _OPTIMIZE_ _ALWAYS_CONSTEXPR_ inline explicit Qty(int64_t _value)
        : number((double)_value) {}
    // Constructor with a floating point number
    _OPTIMIZE_ _ALWAYS_CONSTEXPR_ inline explicit Qty(double _value) : number(_value) {}
    // Calculate a multiplicative conversion factor to convert from
    // the current unit to the unit of the given argument
    template <UTxt V>
    _OPTIMIZE_ _ALWAYS_CONSTEXPR_ inline double
    conversionFactorTo(const Qty<V>) const {
        return conversionFactor(Qty<U>::unitDef,Qty<V>::unitDef);
    }

    // Generic unit conversion operator, _CONSTEXPR_ will be used automatically
    template <UTxt V> _OPTIMIZE_ _CONSTEXPR_ inline operator Qty<V>() const {
        // Calculate a conversion factor
        _CONSTEXPR_ auto convFactor = conversionFactor(Qty<U>::unitDef, Qty<V>::unitDef);
        // Return a completely new object with the requested unit
        // here we multiply the value by the conversion factor
        // with the caveat that the value may not be _CONSTEXPR_
        // so the compiler may issue a single multiplication
        // to do the conversion at run time
        return Qty<V>{number * convFactor};
    }

    // Addition operator for 2 units
    // Convention: Resulting unit of an addition or subtraction is always the unit
    // of the left argument This is done to guarantee that adding with +=
    // operations do not change the unit of the result
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline Qty<U> &operator+=(const Qty<V> rhs) {
        const auto convFactor = conversionFactor(Qty<V>::unitDef, Qty<U>::unitDef);
        number += rhs.number * convFactor;
        return *this;
    }
    // Subtraction operator for 2 units
    // Convention: Resulting unit of an addition or subtraction is always the unit
    // of the left argument This is done to guarantee that adding with +=
    // operations do not change the unit of the result
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline Qty<U> &operator-=(const Qty<V> rhs) {
        const auto convFactor = conversionFactor(Qty<V>::unitDef,Qty<U>::unitDef);
        number -= rhs.number * convFactor;
        return *this;
    }

    // Multiplication w/assignment can only exist with a scalar
    _OPTIMIZE_ _CONSTEXPR_ inline Qty<U> &operator*=(double rhs) {
        number *= rhs;
        return *this;
    }

    _OPTIMIZE_ _CONSTEXPR_ inline Qty<U> &operator/=(double rhs) {
        number *= rhs;
        return *this;
    }

    _OPTIMIZE_ _CONSTEXPR_ inline double value() const { return number; }
    _OPTIMIZE_ _CONSTEXPR_ const char *unit() const { return unitDef.u_def; }

    template <UTxt V> friend class Qty;

    template <UTxt W, UTxt V>
    friend consteval inline double
    conversionFactor(const Qty<W> /*from*/, const Qty<V> /*to*/);

    template <UTxt W, UTxt V>
    friend _CONSTEXPR_ Qty<W> operator+(const Qty<W> lhs, const Qty<V> rhs);

    template <UTxt W, UTxt V>
    friend _CONSTEXPR_ Qty<W> operator-(const Qty<W> lhs, const Qty<V> rhs);

    template <UTxt W, UTxt V>
    friend _CONSTEXPR_ auto operator*(const Qty<W> lhs, const Qty<V> rhs);

    template <UTxt W, UTxt V>
    friend _CONSTEXPR_ auto operator/(const Qty<W> lhs, const Qty<V> rhs);

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const int lhs,
                                                          const Qty<V> rhs);

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const int64_t lhs,
                                               const Qty<V> rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const double lhs,
                                               const Qty<V> rhs);
    // Operator *= or /= with another unit does not exist, a variable cannot
    // change units once declared. For that use case, use the RQty class.
    template <int64_t EXP, UTxt V>
    friend _CONSTEXPR_ inline auto pow(const Qty<V> lhs);
    template <int64_t NUM, int64_t DEN, UTxt V>
    friend _CONSTEXPR_ inline auto pow(const Qty<V> lhs);

#ifdef RUNTIME_COMPONENT

    friend class RQty;

    _CONSTEXPR_ operator RQty() const;

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator*(const RQty &lhs, const Qty<V> &rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator*(const Qty<V> &lhs, const RQty &rhs);

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const RQty &lhs, const Qty<V> &rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const Qty<V> &lhs, const RQty &rhs);

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator+(const RQty &lhs, const Qty<V> &rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator-(const RQty &lhs, const Qty<V> &rhs);


#endif

private:
    // This becomes the one and only data member: the number
    double number;

    // The actual unitDef for the physical quantity is a static _CONSTEXPR_ member
    // of the class therefore the compiler will not create/store any data unless
    // it is used during run time
    static constexpr const UnitDefinition unitDef = {U};
};


// Calculate a multiplicative conversion factor to convert from
// the current unit to the unit of the given argument
_OPTIMIZE_ consteval inline double
conversionFactor(const UnitDefinition from, const UnitDefinition to) {
    // To convert a unit from U to V:
    // k*U = k*U* (expand(U)/U) * (V/expand(V)) = k*V* (expand(U)/expand(V))
    // k*U = k*V * (expand(U)/expand(V))
    // conv. factor = (expand(U)/expand(V))

    // Create a unit V with a value of 1 that is a _CONSTEXPR_
    // We cannot use the one from the argument because the quantity may not be
    // _CONSTEXPR_ even though the unit of the quantity always is
    const auto quotientUnit =
        from.divide(to); // This is (U / V)
    double convFactor = 1.0;
    double powerOf10 = 1.0;
    {
        const auto combinedUnit =
            quotientUnit.simplify(); // This makes expand(U/V) ==
        // expand(U)/expand(V) == conversion factor
        // After simplification, the result should be a non-dimensional
        // factor, otherwise the units are incompatible and we cannot convert
        // Check that the resulting unit has an 'empty' list of tokens
        // (therefore non-dimensional)
        if (combinedUnit.definition[0].tokEnd !=
            combinedUnit.definition[0].tokStart) {
            // Also check that there were no errors
            if (combinedUnit.error_state != UnitError::NoError) {
                throw UnitErrorMessages[combinedUnit.error_state];
            }
            // This isn't a real throw, just generates a compile error. The
            // string will be buried in other messages but still visible by the
            // user
            throw "Incompatible Units";
        }

        // Units were compatible, so extract the value
        convFactor = (combinedUnit.value_den != 1)
                         ? ((double)combinedUnit.value_ip) /
                               combinedUnit.value_den
                         : combinedUnit.value_ip;
        // Apply the exponent
        powerOf10 = powerOf10Exponent(combinedUnit.value_exp);
    }
    // And we have the conversion factor
    return convFactor * powerOf10;
}


// Calculate a multiplicative conversion factor to convert from
// the current unit to the unit of the given argument
template <UTxt U, UTxt V>
_OPTIMIZE_ consteval inline double
conversionFactor(const Qty<U> /*from*/, const Qty<V> /*to*/) {
    return conversionFactor(Qty<U>::unitDef,Qty<V>::unitDef);
}

// Addition operator for 2 units
// Convention: Resulting unit of an addition or subtraction is always the unit
// of the left argument This is done to guarantee that adding with += operations
// do not change the unit of the result
template <UTxt W, UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<W> operator+(const Qty<W> lhs,
                                               const Qty<V> rhs) {
    const auto convFactor = conversionFactor(Qty<V>::unitDef, Qty<W>::unitDef);
    double finalvalue = lhs.value() + rhs.value() * convFactor;
    return Qty<W>{finalvalue};
}

template <UTxt U, UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<U> operator-(const Qty<U> lhs,
                                               const Qty<V> rhs) {
    const auto convFactor = conversionFactor(Qty<V>::unitDef, Qty<U>::unitDef);
    double finalvalue = lhs.value() - rhs.value() * convFactor;
    return Qty<U>{finalvalue};
}

// Multiplication operator for 2 units
// No unit conversion is performed, the values are simply multiplied
// The resulting unit is not simplified in any way
// except identical tokens will merge their exponents
// For example: m*m^2 == m^3 but m*cm == m*cm (the SI prefixed unit is
// treated as a different unit)
template <UTxt U, UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator*(const Qty<U> lhs,
                                             const Qty<V> rhs) {
    // Guarantee all _CONSTEXPR_ constants so the operation is fully constevaled
    // even if the arguments have unknown values at compile time
    constexpr const auto finalUnit =
        Qty<U>::unitDef.multiply(Qty<V>::unitDef).update();
    constexpr const auto finalUnitDefinition =
        to_UTxt<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    return Qty<finalUnitDefinition>{lhs.number * rhs.number};
}

template <UTxt U, UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const Qty<U> lhs,
                                             const Qty<V> rhs) {
    // Guarantee all _CONSTEXPR_ constants so the operation is fully constevaled
    // even if the arguments have unknown values at compile time
    _CONSTEXPR_ Qty<U> lhsUnit{1.0};
    _CONSTEXPR_ Qty<V> rhsUnit{1.0};
    _CONSTEXPR_ auto finalUnit = lhsUnit.unitDef.divide(rhsUnit.unitDef).update();
    constexpr auto finalUnitDefinition = to_UTxt<finalUnit.u_defLen>(finalUnit);

    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() / rhs.value();
    return Qty<finalUnitDefinition>{finalvalue};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator*(const double lhs,
                                               const Qty<V> rhs) {
    return Qty<V>{lhs*rhs.value()};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator*(const int64_t lhs,
                                               const Qty<V> rhs) {
    double finalvalue = lhs * rhs.value();
    return Qty<V>{finalvalue};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator*(const int lhs,
                                               const Qty<V> rhs) {
    double finalvalue = lhs * rhs.value();
    return Qty<V>{finalvalue};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator*(const Qty<V> lhs,
                                               const double rhs) {
    return Qty<V>{rhs * lhs.value()};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator*(const Qty<V> lhs,
                                               const int64_t rhs) {
    return Qty<V>{rhs * lhs.value()};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator*(const Qty<V> lhs,
                                               const int rhs) {
    double finalvalue = rhs * lhs.value();
    return Qty<V>{finalvalue};
}


// Division operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const double lhs,
                                               const Qty<V> rhs) {
    constexpr const auto finalUnit =
        Qty<V>::unitDef.invert().update();
    constexpr const auto finalUnitDefinition =
        to_UTxt<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    return Qty<finalUnitDefinition>{lhs / rhs.value()};
}

// Division operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const int64_t lhs,
                                               const Qty<V> rhs) {
    constexpr const auto finalUnit =
        Qty<V>::unitDef.invert().update();
    constexpr const auto finalUnitDefinition =
        to_UTxt<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    return Qty<finalUnitDefinition>{lhs / rhs.value()};
}

// Division operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const int lhs,
                                               const Qty<V> rhs) {
    constexpr const auto finalUnit =
        Qty<V>::unitDef.invert().update();
    constexpr const auto finalUnitDefinition =
        to_UTxt<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    return Qty<finalUnitDefinition>{lhs / rhs.value()};
}

// Division operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator/(const Qty<V> lhs,
                                               const double rhs) {
    return Qty<V>{lhs.value() / rhs};
}

// Division operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator/(const Qty<V> lhs,
                                               const int64_t rhs) {
    return Qty<V>{lhs.value() / rhs};
}

// Division operator by a non-dimensional scalar
// Simply preserves the original unit
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline Qty<V> operator/(const Qty<V> lhs,
                                               const int rhs) {
    return Qty<V>{lhs.value() / rhs};
}


constexpr int64_t toConstExpr(const int64_t num) { return num; }

template <int64_t EXP, UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto pow(const Qty<V> lhs) {
    constexpr auto finalUnit =
        Qty<V>::unitDef.pow(EXP,1).update();
    constexpr auto finalUnitDefinition =
        to_UTxt<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    return Qty<finalUnitDefinition>{intpow(lhs.value(),EXP)};
}

template <int64_t NUM, int64_t DEN, UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto pow(const Qty<V> lhs) {
    constexpr auto finalUnit =
        Qty<V>::unitDef.pow(NUM,DEN).update();
    constexpr auto finalUnitDefinition =
        to_UTxt<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    return Qty<finalUnitDefinition>{intpow(introot(lhs.value(),(double)DEN),(double)NUM)};
}

template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto sqrt(const Qty<V> lhs) {
    return pow<1,2>(lhs);
}
// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

// This is the runtime component 'Quantity' class, with a string defined at run
// time that represents the unit definition
#ifdef RUNTIME_COMPONENT

#include <stdexcept>
#include <string>

class RQty {
public:
    // Default constructor creates the non-dimensional number 0.0
    _OPTIMIZE_ _CONSTEXPR_ inline RQty() : number(0.0), unitDef() {}
    template <size_t N>
    _OPTIMIZE_ _CONSTEXPR_ inline RQty(double _value, const char (&_unit)[N])
        : number(_value), unitDef("", _unit) {}

    _OPTIMIZE_ _CONSTEXPR_ inline RQty(double _value,
                                       const std::string &_unit)
        : number((double)_value), unitDef(_unit.c_str(), _unit.size()) {}

private:
    // Constructor used only internally
    _OPTIMIZE_ _CONSTEXPR_ inline RQty(double _value, const UnitDefinition &_udef)
        : number(_value), unitDef(_udef) {}

public:
    // Calculate a multiplicative conversion factor to convert from
    // the current unit to the unit of the given argument
    _OPTIMIZE_ _CONSTEXPR_ inline double
    conversionFactorTo(const RQty &unitTo) const {
        // To convert a unit from U to V:
        // k*U = k*U* (expand(U)/U) * (V/expand(V)) = k*V* (expand(U)/expand(V))
        // k*U = k*V * (expand(U)/expand(V))
        // conv. factor = (expand(U)/expand(V))

        // Create a unit V with a value of 1 that is a _CONSTEXPR_
        // We cannot use the one from the argument because the quantity may not be
        // _CONSTEXPR_ even though the unit of the quantity always is
        auto quotientUnit = unitDef.divide(unitTo.unitDef); // This is (U / V)
        auto combinedUnit = quotientUnit.simplify(); // This makes expand(U/V) ==
        // expand(U)/expand(V) == conversion factor
        // After simplification, the result should be a non-dimensional factor,
        // otherwise the units are incompatible and we cannot convert Check that the
        // resulting unit has an 'empty' list of tokens (therefore non-dimensional)
        if (combinedUnit.definition[0].tokEnd !=
            combinedUnit.definition[0].tokStart) {
            // Also check that there were no errors
            if (combinedUnit.error_state != UnitError::NoError) {
                throw std::runtime_error(UnitErrorMessages[combinedUnit.error_state]);
            }
            throw std::runtime_error("Incompatible Units");
        }
        // Units were compatible, so extract the value
        double convFactor =
            (combinedUnit.value_den != 1)
                                     ? ((double)combinedUnit.value_ip) / combinedUnit.value_den
                                     : combinedUnit.value_ip;
        // Apply the exponent
        const double powerOf10 = powerOf10Exponent(combinedUnit.value_exp);
        // And we have the conversion factor
        return convFactor * powerOf10;
    }
    // Calculate a multiplicative conversion factor to convert from
    // the current unit to the unit of the given argument
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline double
    conversionFactorTo(const Qty<V> unitTo) const {
        // To convert a unit from U to V:
        // k*U = k*U* (expand(U)/U) * (V/expand(V)) = k*V* (expand(U)/expand(V))
        // k*U = k*V * (expand(U)/expand(V))
        // conv. factor = (expand(U)/expand(V))

        // Create a unit V with a value of 1 that is a _CONSTEXPR_
        // We cannot use the one from the argument because the quantity may not be
        // _CONSTEXPR_ even though the unit of the quantity always is
        auto quotientUnit = unitDef.divide(unitTo.unitDef); // This is (U / V)
        auto combinedUnit = quotientUnit.simplify(); // This makes expand(U/V) ==
        // expand(U)/expand(V) == conversion factor
        // After simplification, the result should be a non-dimensional factor,
        // otherwise the units are incompatible and we cannot convert Check that the
        // resulting unit has an 'empty' list of tokens (therefore non-dimensional)
        if (combinedUnit.definition[0].tokEnd !=
            combinedUnit.definition[0].tokStart) {
            // Also check that there were no errors
            if (combinedUnit.error_state != UnitError::NoError) {
                throw std::runtime_error(UnitErrorMessages[combinedUnit.error_state]);
            }
            throw std::runtime_error("Incompatible Units");
        }
        // Units were compatible, so extract the value
        double convFactor =
            (combinedUnit.value_den != 1)
                                     ? ((double)combinedUnit.value_ip) / combinedUnit.value_den
                                     : combinedUnit.value_ip;
        // Apply the exponent
        const double powerOf10 = powerOf10Exponent(combinedUnit.value_exp);
        // And we have the conversion factor
        return convFactor * powerOf10;
    }
    // Calculate a multiplicative conversion factor to convert from
    // the given unit to the current unit
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline double
    conversionFactorFrom(const Qty<V> &unitFrom) const {
        // To convert a unit from U to V:
        // k*U = k*U* (expand(U)/U) * (V/expand(V)) = k*V* (expand(U)/expand(V))
        // k*U = k*V * (expand(U)/expand(V))
        // conv. factor = (expand(U)/expand(V))

        // Create a unit V with a value of 1 that is a _CONSTEXPR_
        // We cannot use the one from the argument because the quantity may not be
        // _CONSTEXPR_ even though the unit of the quantity always is
        auto quotientUnit = unitFrom.unitDef.divide(unitDef); // This is (U / V)
        auto combinedUnit = quotientUnit.simplify(); // This makes expand(U/V) ==
        // expand(U)/expand(V) == conversion factor
        // After simplification, the result should be a non-dimensional factor,
        // otherwise the units are incompatible and we cannot convert Check that the
        // resulting unit has an 'empty' list of tokens (therefore non-dimensional)
        if (combinedUnit.definition[0].tokEnd !=
            combinedUnit.definition[0].tokStart) {
            // Also check that there were no errors
            if (combinedUnit.error_state != UnitError::NoError) {
                throw std::runtime_error(UnitErrorMessages[combinedUnit.error_state]);
            }
            throw std::runtime_error("Incompatible Units");
        }
        // Units were compatible, so extract the value
        double convFactor =
            (combinedUnit.value_den != 1)
                                     ? ((double)combinedUnit.value_ip) / combinedUnit.value_den
                                     : combinedUnit.value_ip;
        // Apply the exponent
        const double powerOf10 = powerOf10Exponent(combinedUnit.value_exp);
        // And we have the conversion factor
        return convFactor * powerOf10;
    }

    // Generic unit conversion operator
    template <UTxt V> _OPTIMIZE_ _CONSTEXPR_ inline operator Qty<V>() const {
        // Create guaranteed _CONSTEXPR_ units for source and destination
        _CONSTEXPR_ Qty<V> unitV{1.0};
        // Calculate a conversion factor
        auto convFactor = conversionFactorTo(unitV);
        // Return a completely new object with the requested unit
        return Qty<V>{number * convFactor};
    }

    // Operations with other run time units
    // Assignment will not CONVERT units, will copy the units of the other object
    // To convert to a fixed set of units, assign it to a Qty<U>
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator=(const RQty &rhs) {
        number = rhs.number;
        unitDef = rhs.unitDef;
        return *this;
    }

    // Convention: Resulting unit of an addition or subtraction is always the unit
    // of the left argument This is done to guarantee that adding with +=
    // operations do not change the unit of the result
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator+=(const Qty<V> &rhs) {
        const auto convFactor = conversionFactorFrom(rhs);
        number += rhs.number * convFactor;
        return *this;
    }
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator+=(const RQty &rhs) {
        const auto convFactor = rhs.conversionFactorTo(*this);
        number += rhs.number * convFactor;
        return *this;
    }
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator+=(const double rhs) {
        // We'll only allow this if the destination unit is compattible with
        // nondimensional values
        constexpr Qty<""> nonDimensional{1.0};
        // Conversion will throw "Incompatible units" error unless our unit is
        // non-dimensional
        const auto convFactor = conversionFactorFrom(nonDimensional);
        number += rhs * convFactor;
        return *this;
    }

    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator-=(const Qty<V> &rhs) {
        const auto convFactor = conversionFactorFrom(rhs);
        number -= rhs.number * convFactor;
        return *this;
    }
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator-=(const RQty &rhs) {
        const auto convFactor = rhs.conversionFactorTo(*this);
        number -= rhs.number * convFactor;
        return *this;
    }
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator-=(const double rhs) {
        // We'll only allow this if the destination unit is compattible with
        // nondimensional values
        constexpr Qty<""> nonDimensional{1.0};
        // Conversion will throw "Incompatible units" error unless our unit is
        // non-dimensional
        const auto convFactor = conversionFactorFrom(nonDimensional);
        number -= rhs * convFactor;
        return *this;
    }

    // Multiplication w/assignment
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator*=(double rhs) {
        number *= rhs;
        return *this;
    }
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator*=(const RQty &rhs) {
        auto finalUnit = unitDef.multiply(rhs.unitDef).update();
        unitDef = finalUnit;
        number *= rhs.value();
        return *this;
    }
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator*=(const Qty<V> &rhs) {
        auto finalUnit = unitDef.multiply(rhs.unitDef).update();
        unitDef = finalUnit;
        number *= rhs.value();
        return *this;
    }

    // Division w/assignment
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator/=(double rhs) {
        number /= rhs;
        return *this;
    }

    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator/=(const RQty &rhs) {
        auto finalUnit = unitDef.divide(rhs.unitDef).update();
        unitDef = finalUnit;
        number /= rhs.value();
        return *this;
    }
    template <UTxt V>
    _OPTIMIZE_ _CONSTEXPR_ inline RQty &operator/=(const Qty<V> &rhs) {
        auto finalUnit = unitDef.divide(rhs.unitDef).update();
        unitDef = finalUnit;
        number /= rhs.value();
        return *this;
    }

    _OPTIMIZE_ _CONSTEXPR_ inline double value() const { return number; }
    const char *unit() { return unitDef.u_def; }

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator*(const RQty &lhs, const Qty<V> &rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator*(const Qty<V> &lhs, const RQty &rhs);
    friend _CONSTEXPR_ inline auto operator*(const RQty &lhs, const RQty &rhs);
    friend _CONSTEXPR_ inline auto operator*(const double lhs,
                                             const RQty &rhs);
    friend _CONSTEXPR_ inline auto operator*(const RQty &lhs,
                                             const double rhs);

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const RQty &lhs, const Qty<V> &rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator/(const Qty<V> &lhs, const RQty &rhs);
    friend _CONSTEXPR_ inline auto operator/(const RQty &lhs, const RQty &rhs);
    friend _CONSTEXPR_ inline auto operator/(const double lhs,
                                             const RQty &rhs);
    friend _CONSTEXPR_ inline auto operator/(const RQty &lhs,
                                             const double rhs);

    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator+(const RQty &lhs, const Qty<V> &rhs);
    friend _CONSTEXPR_ inline auto operator+(const RQty &lhs, const RQty &rhs);
    template <UTxt V>
    friend _CONSTEXPR_ inline auto operator-(const RQty &lhs, const Qty<V> &rhs);
    friend _CONSTEXPR_ inline auto operator-(const RQty &lhs, const RQty &rhs);

    template <UTxt V> friend class Qty;

private:
    double number;
    UnitDefinition unitDef;
};

// Addition operator for 2 units
// Convention: Resulting unit of an addition or subtraction is always the unit
// of the left argument This is done to guarantee that adding with += operations
// do not change the unit of the result
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator+(const RQty &lhs,
                                             const Qty<V> &rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    const double finalvalue = lhs.value() + rhs.value() * convFactor;
    return RQty{finalvalue, lhs.unitDef};
}
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator+(const Qty<V> &lhs,
                                             const RQty &rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    const double finalvalue = lhs.value() + rhs.value() * convFactor;
    return Qty<V>{finalvalue};
}
_OPTIMIZE_ _CONSTEXPR_ inline auto operator+(const RQty &lhs, const RQty &rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    const double finalvalue = lhs.value() + rhs.value() * convFactor;
    return RQty{finalvalue, lhs.unitDef};
}

template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator-(const RQty &lhs,
                                             const Qty<V> &rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    const double finalvalue = lhs.value() - rhs.value() * convFactor;
    return RQty{finalvalue, lhs.unitDef};
}
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator-(const Qty<V> &lhs,
                                             const RQty &rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    const double finalvalue = lhs.value() - rhs.value() * convFactor;
    return Qty<V>{finalvalue};
}
_OPTIMIZE_ _CONSTEXPR_ inline auto operator-(const RQty &lhs, const RQty &rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    const double finalvalue = lhs.value() - rhs.value() * convFactor;
    return RQty{finalvalue, lhs.unitDef};
}

// Multiplication operator for 2 units
// No unit conversion is performed, the values are simply multiplied
// The resulting unit is not simplified in any way
// except identical tokens will merge their exponents
// For example: m*m^2 == m^3 but m*cm == m*cm (the SI prefixed unit is
// treated as a different unit)
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator*(const RQty &lhs,
                                             const Qty<V> &rhs) {
    auto finalUnit = lhs.unitDef.multiply(rhs.unitDef).update();
    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() * rhs.value();
    return RQty{finalvalue, finalUnit};
}
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator*(const Qty<V> &lhs,
                                             const RQty &rhs) {
    auto finalUnit = lhs.unitDef.multiply(rhs.unitDef).update();
    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() * rhs.value();
    return RQty{finalvalue, finalUnit};
}
_OPTIMIZE_ _CONSTEXPR_ inline auto operator*(const RQty &lhs, const RQty &rhs) {
    auto finalUnit = lhs.unitDef.multiply(rhs.unitDef).update();
    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() * rhs.value();
    return RQty{finalvalue, finalUnit};
}

template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const RQty &lhs,
                                             const Qty<V> &rhs) {
    auto finalUnit = lhs.unitDef.divide(rhs.unitDef).update();
    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() / rhs.value();
    return RQty{finalvalue, finalUnit};
}
template <UTxt V>
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const Qty<V> &lhs,
                                             const RQty &rhs) {
    auto finalUnit = lhs.unitDef.divide(rhs.unitDef).update();
    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() * rhs.value();
    return RQty{finalvalue, finalUnit};
}
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const RQty &lhs, const RQty &rhs) {
    auto finalUnit = lhs.unitDef.divide(rhs.unitDef).update();
    // This it the only operation the compiler will do at run time if needed
    double finalvalue = lhs.value() / rhs.value();
    return RQty{finalvalue, finalUnit};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
_OPTIMIZE_ _CONSTEXPR_ inline auto operator*(const double lhs,
                                             const RQty &rhs) {
    double finalvalue = lhs * rhs.value();
    return RQty{finalvalue, rhs.unitDef};
}
_OPTIMIZE_ _CONSTEXPR_ inline auto operator*(const RQty &lhs,
                                             const double rhs) {
    double finalvalue = rhs * lhs.value();
    return RQty{finalvalue, lhs.unitDef};
}
// Division operator by a non-dimensional scalar
// Simply preserves (or inverts, depending on order of arguments)
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const double lhs,
                                             const RQty &rhs) {
    auto finalUnit = rhs.unitDef.invert().update();
    double finalvalue = lhs / rhs.value();
    return RQty{finalvalue, finalUnit};
}
_OPTIMIZE_ _CONSTEXPR_ inline auto operator/(const RQty &lhs,
                                             const double rhs) {
    double finalvalue = lhs.value() / rhs;
    return RQty{finalvalue, lhs.unitDef};
}

template <UTxt U> _OPTIMIZE_ _CONSTEXPR_ inline Qty<U>::operator RQty() const {
    // Create guaranteed _CONSTEXPR_ units for source and destination
    return RQty{number, unitDef};
}

#endif
