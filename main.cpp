#include <cmath>
#include <iostream>

#include "fraction_units.hpp"

#include "external/test/include/acutest.h"

auto massFlow(const Qty<"kg/m^3"> density, const Qty<"m^2"> area, const Qty<"m/s"> velocity) {
    return density * area * velocity;
}

double cosUnit(const Qty<"r"> angle) {
    return std::cos(angle.value());
}


void test_basicmath1()
{
    // Testing custom power function because std::pow is not constexpr in MSVC and Clang yet
    double x = 123.4;

    for(auto i=0;i<100;++i) {
        double y = intpow(x,i);
        TEST_CHECK(std::abs(y - std::pow(x,i))<1e-15);
    }
}

void test_basicmath2()
{
    // Testing custom power function because std::pow is not constexpr in MSVC and Clang yet
    double x = 123.4;

    for(auto i=1;i<100;++i) {
        double y = introot(x,i);
        TEST_CHECK(std::abs(y - std::pow(x,1.0/i))<1e-15);
        if(std::abs(y - std::pow(x,1.0/i))>1e-15) {
            std::cout << "Fail!" <<std::endl;
        }
    }

    x = 123456789123456789.0;

    for(auto i=1;i<100;++i) {
        double y = introot(x,i);
        const double tolerance = std::pow(10,log10(y)-15);
        TEST_CHECK(std::abs(y - std::pow(x,1.0/i))<tolerance);
        if(std::abs(y - std::pow(x,1.0/i))>tolerance) {
            std::cout << "Fail!" <<std::endl;
        }
    }

    x = 0.123123123;

    for(auto i=1;i<100;++i) {
        double y = introot(x,i);
        const double tolerance = std::pow(10,log10(y)-15);
        TEST_CHECK(std::abs(y - std::pow(x,1.0/i))<tolerance);
        if(std::abs(y - std::pow(x,1.0/i))>tolerance) {
            std::cout << "Fail!" <<std::endl;
        }
    }
}

void test_basicmath3()
{
    // Testing custom power function because std::pow is not constexpr in MSVC and Clang yet
    double x = 123.4;

    double m = dbl_mantissa(x);
    int exp = dbl_exponent(x);

    TEST_CHECK(exp == 6);
    TEST_CHECK(m == 1.928125);

    TEST_CHECK(m * (1<<exp) == x);

    auto y = dbl_make(m,exp+2);

    TEST_CHECK(y == x*4);
}

void test_UnitDefinition_parser()
{
    // Try creating a unit, then print all tokens and exponents
    UnitDefinition check("check","kg/ft^3");

    TEST_CHECK(check.error_state==UnitError::NoError);
    // kg
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==2);
    TEST_CHECK(check.definition[0].expDen==1);
    TEST_CHECK(check.definition[0].expNum==1);
    // ft^-3
    TEST_CHECK(check.definition[1].tokStart==3);
    TEST_CHECK(check.definition[1].tokEnd==5);
    TEST_CHECK(check.definition[1].expDen==1);
    TEST_CHECK(check.definition[1].expNum==-3);
    // Value = 1/1*10^0
    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);
}

void test_UnitDefinition_parser_number()
{
    // Parse a simple number
    UnitDefinition check("check","1.2345");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);
    // Value = 12345/1 * 10^-4
    TEST_CHECK(check.value_ip==12345);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-4);
}

void test_UnitDefinition_parser_number2()
{
    // Parse a number with denominator
    UnitDefinition check("check","1.2345/567");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);
    // Value = 12345/567 * 10^-4 but fraction should be simplified
    // by dividing by 3
    TEST_CHECK(check.value_ip==4115);
    TEST_CHECK(check.value_den==189);
    TEST_CHECK(check.value_exp==-4);
}

void test_UnitDefinition_parser_number3()
{
    // Parse a number, denominator is multiple of 10
    UnitDefinition check("check","1.2345/500");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==2469);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-6);
}

void test_UnitDefinition_parser_number4()
{
    // Parse a number, numerator is multiple of 10
    UnitDefinition check("check","1.234500/567");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==4115);
    TEST_CHECK(check.value_den==189);
    TEST_CHECK(check.value_exp==-4);
}

void test_UnitDefinition_parser_number5()
{
    // Parse a number, numerator doesn't exist
    UnitDefinition check("check","/567");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==567);
    TEST_CHECK(check.value_exp==0);
}

void test_UnitDefinition_parser_number6()
{
    // Parse a number, denominator doesn't exist
    UnitDefinition check("check","1.2345/");

    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number7()
{
    // Parse a number, strange character
    UnitDefinition check("check","1.23#45");

    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number8()
{
    // Parse a number, exponent
    UnitDefinition check("check","1.23e2");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==123);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);
}

void test_UnitDefinition_parser_number9()
{
    // Parse a number, negative exponent
    UnitDefinition check("check","1.23e-2");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==123);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-4);
}

void test_UnitDefinition_parser_number10()
{
    // Parse a number, exponent on denominator
    UnitDefinition check("check","1/1.23e-2");

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==123);
    TEST_CHECK(check.value_exp==4);
}

void test_UnitDefinition_parser_number11()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23e--2");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number12()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23eE2");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number13()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23e-2.5");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number14()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","e-2.5");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number15()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","/e-2.5");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number16()
{
    // Parse a number, invalid exponent should be considered a unit
    UnitDefinition check("check","1.23e");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==123);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-2);

    // 'e'
    TEST_CHECK(check.definition[0].tokStart==4);
    TEST_CHECK(check.definition[0].tokEnd==5);
    TEST_CHECK(check.definition[0].expNum==1);
    TEST_CHECK(check.definition[0].expDen==1);

}

void test_UnitDefinition_parser_number17()
{
    // Parse a number, invalid exponent right before the underscore
    // Should be a unit 'e_m', this is tricky, maybe the underscore should not be permitted in tokens
    UnitDefinition check("check","1.23e_m");

    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==123);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-2);

    // 'e_m'
    TEST_CHECK(check.definition[0].tokStart==4);
    TEST_CHECK(check.definition[0].tokEnd==7);
    TEST_CHECK(check.definition[0].expNum==1);
    TEST_CHECK(check.definition[0].expDen==1);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[1].tokEnd==0);

}

void test_UnitDefinition_parser_number18()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23e/567");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number19()
{
    // Expressions not allowed by themselves in the middle of a unit
    UnitDefinition check("check","1.23*4");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_number20()
{
    // Expressions not allowed by themselves in the middle of a unit
    UnitDefinition check("check","1.234(5)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent1()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==1);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_unitExponent2()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^2/3");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==3);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_unitExponent3()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^-2/3");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==-2);
    TEST_CHECK(check.definition[0].expDen==3);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_unitExponent4()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2/4)");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==4);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_unitExponent5()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^-(2/3)");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==-2);
    TEST_CHECK(check.definition[0].expDen==3);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_unitExponent6()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^2/-3");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==-2);
    TEST_CHECK(check.definition[0].expDen==3);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_unitExponent7()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2+1)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent8()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2*3)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent9()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2 3)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);

}
void test_UnitDefinition_parser_unitExponent10() {
    UnitDefinition check("check", "m^( 2/)");
    TEST_CHECK(check.error_state == UnitError::InvalidDefinition);
}
void test_UnitDefinition_parser_unitExponent11()
{
    UnitDefinition check("check", "m^(2/ )");
    TEST_CHECK(check.error_state == UnitError::InvalidDefinition);
}
void test_UnitDefinition_parser_unitExponent12()
{
    UnitDefinition check("check", "m^( / 3 )");
    TEST_CHECK(check.error_state == UnitError::InvalidDefinition);
}
void test_UnitDefinition_parser_unitExponent13() {
    UnitDefinition check("check", "m^( 2 / 3 )");
    TEST_CHECK(check.error_state == UnitError::NoError);

    TEST_CHECK(check.value_ip == 1);
    TEST_CHECK(check.value_den == 1);
    TEST_CHECK(check.value_exp == 0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart == 0);
    TEST_CHECK(check.definition[0].tokEnd == 1);
    TEST_CHECK(check.definition[0].expNum == 2);
    TEST_CHECK(check.definition[0].expDen == 3);
    // End
    TEST_CHECK(check.definition[1].tokStart == 0);
    TEST_CHECK(check.definition[2].tokEnd == 0);
}

void test_UnitDefinition_parser_unitExponent14()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2_3)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent15()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2e3)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent16()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^(2/3");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent17()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^2.3");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent18()
{
    // Parse a unit exponent
    constexpr UnitDefinition check("check","m^123456789012345678901234567890");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent19()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^1/123456789012345678901234567890");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent20()
{
    // Parse a unit exponent
    UnitDefinition check("check","m^/12");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_unitExponent21()
{
    // Parse a unit exponent
    UnitDefinition check("check","m123");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}

void test_UnitDefinition_parser_multipleTokens1()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m s");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==1);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==2);
    TEST_CHECK(check.definition[1].tokEnd==3);
    TEST_CHECK(check.definition[1].expNum==1);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}

void test_UnitDefinition_parser_multipleTokens2()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m/s^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==1);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==2);
    TEST_CHECK(check.definition[1].tokEnd==3);
    TEST_CHECK(check.definition[1].expNum==-2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}
void test_UnitDefinition_parser_multipleTokens3()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^2/s^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==4);
    TEST_CHECK(check.definition[1].tokEnd==5);
    TEST_CHECK(check.definition[1].expNum==-2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}
void test_UnitDefinition_parser_multipleTokens4()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^2/3s^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==3);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==5);
    TEST_CHECK(check.definition[1].tokEnd==6);
    TEST_CHECK(check.definition[1].expNum==2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}
void test_UnitDefinition_parser_multipleTokens5()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^2/ 3s^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==3);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==6);
    TEST_CHECK(check.definition[1].tokEnd==7);
    TEST_CHECK(check.definition[1].expNum==2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}
void test_UnitDefinition_parser_multipleTokens6()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^(2/-3)s^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==-2);
    TEST_CHECK(check.definition[0].expDen==3);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==8);
    TEST_CHECK(check.definition[1].tokEnd==9);
    TEST_CHECK(check.definition[1].expNum==2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}

void test_UnitDefinition_parser_multipleTokens7()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^(2/s^2)");
    TEST_CHECK(check.error_state==UnitError::InvalidDefinition);
}
void test_UnitDefinition_parser_multipleTokens8()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^2/ s^2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==5);
    TEST_CHECK(check.definition[1].tokEnd==6);
    TEST_CHECK(check.definition[1].expNum==-2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}
void test_UnitDefinition_parser_multipleTokens9()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","m^2/(s^2)");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==1);
    TEST_CHECK(check.definition[0].expNum==2);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==5);
    TEST_CHECK(check.definition[1].tokEnd==6);
    TEST_CHECK(check.definition[1].expNum==-2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}
void test_UnitDefinition_parser_multipleTokens10()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","1/(m^2/(s^2))");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==3);
    TEST_CHECK(check.definition[0].tokEnd==4);
    TEST_CHECK(check.definition[0].expNum==-2);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==8);
    TEST_CHECK(check.definition[1].tokEnd==9);
    TEST_CHECK(check.definition[1].expNum==2);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);
}

void test_UnitDefinition_parser_multipleTokens11()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","1/(m^2/(s^2))*s^-2");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==0);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==3);
    TEST_CHECK(check.definition[0].tokEnd==4);
    TEST_CHECK(check.definition[0].expNum==-2);
    TEST_CHECK(check.definition[0].expDen==1);
    // 's'
    TEST_CHECK(check.definition[1].tokStart==8);
    TEST_CHECK(check.definition[1].tokEnd==9);
    TEST_CHECK(check.definition[1].expNum==2);
    TEST_CHECK(check.definition[1].expDen==1);
    // 's'
    TEST_CHECK(check.definition[2].tokStart==14);
    TEST_CHECK(check.definition[2].tokEnd==15);
    TEST_CHECK(check.definition[2].expNum==-2);
    TEST_CHECK(check.definition[2].expDen==1);
    // End
    TEST_CHECK(check.definition[3].tokStart==0);
    TEST_CHECK(check.definition[3].tokEnd==0);
}

void test_UnitDefinition_update1()
{
    // Test for rebuilding the text of a unit definition - basic check
    UnitDefinition check("check","1/20_m");
    UnitDefinition inverse = check.invert();
    UnitDefinition updated = inverse.update();

    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==1);
    TEST_CHECK(check.value_den==2);
    TEST_CHECK(check.value_exp==-1);

    // 'm'
    TEST_CHECK(check.definition[0].tokStart==5);
    TEST_CHECK(check.definition[0].tokEnd==6);
    TEST_CHECK(check.definition[0].expNum==1);
    TEST_CHECK(check.definition[0].expDen==1);
    // End
    TEST_CHECK(check.definition[1].tokStart==0);
    TEST_CHECK(check.definition[1].tokEnd==0);


    TEST_CHECK(inverse.error_state==UnitError::NoError);

    TEST_CHECK(inverse.value_ip==2);
    TEST_CHECK(inverse.value_den==1);
    TEST_CHECK(inverse.value_exp==1);

    // 'm'
    TEST_CHECK(inverse.definition[0].tokStart==5);
    TEST_CHECK(inverse.definition[0].tokEnd==6);
    TEST_CHECK(inverse.definition[0].expNum==-1);
    TEST_CHECK(inverse.definition[0].expDen==1);
    // End
    TEST_CHECK(inverse.definition[1].tokStart==0);
    TEST_CHECK(inverse.definition[1].tokEnd==0);

    TEST_CHECK(updated.error_state==UnitError::NoError);

    TEST_CHECK(updated.value_ip==2);
    TEST_CHECK(updated.value_den==1);
    TEST_CHECK(updated.value_exp==1);

    // 'm'
    TEST_CHECK(updated.definition[0].tokStart==5);
    TEST_CHECK(updated.definition[0].tokEnd==6);
    TEST_CHECK(updated.definition[0].expNum==-1);
    TEST_CHECK(updated.definition[0].expDen==1);
    // End
    TEST_CHECK(updated.definition[1].tokStart==0);
    TEST_CHECK(updated.definition[1].tokEnd==0);

    TEST_CHECK(updated.u_def[0]=='2');
    TEST_CHECK(updated.u_def[1]=='0');
    TEST_CHECK(updated.u_def[2]=='_');
    TEST_CHECK(updated.u_def[3]=='1');
    TEST_CHECK(updated.u_def[4]=='/');
    TEST_CHECK(updated.u_def[5]=='m');
    TEST_CHECK(updated.u_def[6]==0);
}

void test_UnitDefinition_update2()
{
    // Parse a unit with more than one token
    UnitDefinition check("check","1/20_m^2*kg/s");
    UnitDefinition inverse = check.invert();
    UnitDefinition updated = inverse.update();

    TEST_CHECK(updated.error_state==UnitError::NoError);

    TEST_CHECK(updated.value_ip==2);
    TEST_CHECK(updated.value_den==1);
    TEST_CHECK(updated.value_exp==1);

    // 's'
    TEST_CHECK(updated.definition[0].tokStart==3);
    TEST_CHECK(updated.definition[0].tokEnd==4);
    TEST_CHECK(updated.definition[0].expNum==1);
    TEST_CHECK(updated.definition[0].expDen==1);
    // 'm'
    TEST_CHECK(updated.definition[1].tokStart==6);
    TEST_CHECK(updated.definition[1].tokEnd==7);
    TEST_CHECK(updated.definition[1].expNum==-2);
    TEST_CHECK(updated.definition[1].expDen==1);
    // 'kg'
    TEST_CHECK(updated.definition[2].tokStart==10);
    TEST_CHECK(updated.definition[2].tokEnd==12);
    TEST_CHECK(updated.definition[2].expNum==-1);
    TEST_CHECK(updated.definition[2].expDen==1);
    // End
    TEST_CHECK(updated.definition[3].tokStart==0);
    TEST_CHECK(updated.definition[3].tokEnd==0);

    TEST_CHECK(updated.u_def[0]=='2');
    TEST_CHECK(updated.u_def[1]=='0');
    TEST_CHECK(updated.u_def[2]=='_');
    TEST_CHECK(updated.u_def[3]=='s');
    TEST_CHECK(updated.u_def[4]=='/');
    TEST_CHECK(updated.u_def[5]=='(');
    TEST_CHECK(updated.u_def[6]=='m');
    TEST_CHECK(updated.u_def[7]=='^');
    TEST_CHECK(updated.u_def[8]=='2');
    TEST_CHECK(updated.u_def[9]=='*');
    TEST_CHECK(updated.u_def[10]=='k');
    TEST_CHECK(updated.u_def[11]=='g');
    TEST_CHECK(updated.u_def[12]==')');
    TEST_CHECK(updated.u_def[13]==0);
}

void test_UnitDefinition_update3()
{
    // Update a unit with fractional number
    UnitDefinition check("check","3/20_m");
    UnitDefinition inverse = check.invert();
    UnitDefinition updated = inverse.update();

    TEST_CHECK(updated.error_state==UnitError::NoError);

    TEST_CHECK(updated.value_ip==2);
    TEST_CHECK(updated.value_den==3);
    TEST_CHECK(updated.value_exp==1);

    // 'm'
    TEST_CHECK(updated.definition[0].tokStart==7);
    TEST_CHECK(updated.definition[0].tokEnd==8);
    TEST_CHECK(updated.definition[0].expNum==-1);
    TEST_CHECK(updated.definition[0].expDen==1);
    // End
    TEST_CHECK(updated.definition[1].tokStart==0);
    TEST_CHECK(updated.definition[1].tokEnd==0);

    TEST_CHECK(updated.u_def[0]=='2');
    TEST_CHECK(updated.u_def[1]=='0');
    TEST_CHECK(updated.u_def[2]=='/');
    TEST_CHECK(updated.u_def[3]=='3');
    TEST_CHECK(updated.u_def[4]=='_');
    TEST_CHECK(updated.u_def[5]=='1');
    TEST_CHECK(updated.u_def[6]=='/');
    TEST_CHECK(updated.u_def[7]=='m');
    TEST_CHECK(updated.u_def[8]==0);
}

void test_UnitDefinition_update4()
{
    // Update a unit with fractional number
    UnitDefinition check("check","25/16_m");
    UnitDefinition square = check.pow(1,2);
    UnitDefinition updated = square.update();

    TEST_CHECK(updated.error_state==UnitError::NoError);

    TEST_CHECK(updated.value_ip==5);
    TEST_CHECK(updated.value_den==4);
    TEST_CHECK(updated.value_exp==0);

    // 'm'
    TEST_CHECK(updated.definition[0].tokStart==4);
    TEST_CHECK(updated.definition[0].tokEnd==5);
    TEST_CHECK(updated.definition[0].expNum==1);
    TEST_CHECK(updated.definition[0].expDen==2);
    // End
    TEST_CHECK(updated.definition[1].tokStart==0);
    TEST_CHECK(updated.definition[1].tokEnd==0);

    TEST_CHECK(updated.u_def[0]=='5');
    TEST_CHECK(updated.u_def[1]=='/');
    TEST_CHECK(updated.u_def[2]=='4');
    TEST_CHECK(updated.u_def[3]=='_');
    TEST_CHECK(updated.u_def[4]=='m');
    TEST_CHECK(updated.u_def[5]=='^');
    TEST_CHECK(updated.u_def[6]=='1');
    TEST_CHECK(updated.u_def[7]=='/');
    TEST_CHECK(updated.u_def[8]=='2');
    TEST_CHECK(updated.u_def[9]==0);
}
void test_UnitDefinition_update5()
    {
    // Same but an irrational number
    UnitDefinition check("check","300/7_m");
    UnitDefinition square = check.pow(1,2);
    UnitDefinition updated = square.update();

    TEST_CHECK(updated.error_state==UnitError::NoError);

    TEST_CHECK(updated.value_ip==3900231685776981);
    TEST_CHECK(updated.value_den==5957702309312746);
    TEST_CHECK(updated.value_exp==2);

    // 'm'
    TEST_CHECK(updated.definition[0].tokEnd==updated.definition[0].tokStart+1);
    TEST_CHECK(updated.definition[0].expNum==1);
    TEST_CHECK(updated.definition[0].expDen==2);
    // End
    TEST_CHECK(updated.definition[1].tokStart==0);
    TEST_CHECK(updated.definition[1].tokEnd==0);

    const char expected[]="3900231685776981e2/5957702309312746_m^1/2";

    for(auto i=0;expected[i]!=0;++i) {
        TEST_CHECK(updated.u_def[i]==expected[i]);
    }
}

void test_Qty_operators1() {
    // Try to use some quantities, this is all constexpr so verify the compiler
    // generates code that simply stores the number and nothing else
    // First test just test defining a constexpr with assignment of the same unit
    Qty<"m"> x = 50.0*_("m");

    TEST_CHECK(x.value() == 50.0);
}

void test_Qty_operators2() {
    // Try to use some quantities, this is all constexpr so verify the compiler
    // generates code that simply stores the number and nothing else
    // First test just test defining a constexpr with assignment of the same unit
    constexpr Qty<"m"> x = 50.0*_("m");

    auto y=x+(10.0*_("cm"));

    // Just storing a value with no conversion should be exact to 'double' precision
    // Since no math operations were performed
    TEST_CHECK(x.value() == 50.0);
    // Unit conversion is done in double precision, so result
    // should be EXACT in double precision, except in very rare corner cases
    // Force a conversion to double so the compiler downgrades to double precision.
    double actual_y = y.value();
    TEST_CHECK(actual_y == 50.1);
}

void test_Qty_operators3() {
    // Try to use some quantities, this is all constexpr so verify the compiler
    // generates code that simply stores the number and nothing else
    // First test just test defining a constexpr with assignment of the same unit
    Qty<"m"> x = 50.0*_("m");

    auto y=x-10.0*_("cm");

    // Just storing a value with no conversion should be exact to 'double' precision
    // Since no math operations were performed
    TEST_CHECK(x.value() == 50.0);
    // Unit conversion is done in double precision, so result
    // should be EXACT in double precision, except in very rare corner cases
    // Force a conversion to double so the compiler downgrades to double precision.
    double actual_y = y.value();
    TEST_CHECK(actual_y == 49.9);
}

void test_Qty_operators4() {
    // Try to use some quantities, this is all constexpr so verify the compiler
    // generates code that simply stores the number and nothing else
    // First test just test defining a constexpr with assignment of the same unit
    Qty<"m"> x = 50.0*_("m");

    // Multiplication operator is different from addition/subtraction in that it
    // creates a new unit from the multiplication of the units. No unit simplification
    // is performed, in this case it will simply multiply the numbers and
    // apply a unit "m*cm"
    const auto y=x*10.0*_("cm");

    // Just storing a value with no conversion should be exact to 'double' precision
    // Since no math operations were performed
    TEST_CHECK(x.value() == 50.0);
    // Unit conversion is done in double precision, so result
    // should be EXACT in double precision, except in very rare corner cases
    // Force a conversion to double so the compiler downgrades to double precision.
    TEST_CHECK(y.value() == 500.0);
    TEST_CHECK(y.unit()[0]=='m');
    TEST_CHECK(y.unit()[1]=='*');
    TEST_CHECK(y.unit()[2]=='c');
    TEST_CHECK(y.unit()[3]=='m');
    TEST_CHECK(y.unit()[4]==0);

}

void test_Qty_operators5() {
    // Try to use some quantities, this is all constexpr so verify the compiler
    // generates code that simply stores the number and nothing else
    // First test just test defining a constexpr with assignment of the same unit
    Qty<"m"> x = 50.0*_("m");

    // Multiplication operator is different from addition/subtraction in that it
    // creates a new unit from the multiplication of the units. No unit simplification
    // is performed, in this case it will simply multiply the numbers and
    // apply a unit "cm*m" since it is NOT commutative
    const auto y=10.0*_("cm")*x;

    // Just storing a value with no conversion should be exact to 'double' precision
    // Since no math operations were performed
    TEST_CHECK(x.value() == 50.0);
    // Unit conversion is done in double precision, so result
    // should be EXACT in double precision, except in very rare corner cases
    // Force a conversion to double so the compiler downgrades to double precision.
    TEST_CHECK(y.value() == 500.0);
    TEST_CHECK(y.unit()[0]=='c');
    TEST_CHECK(y.unit()[1]=='m');
    TEST_CHECK(y.unit()[2]=='*');
    TEST_CHECK(y.unit()[3]=='m');
    TEST_CHECK(y.unit()[4]==0);
}

void test_Qty_operators6() {
    // Comprehensive test of addition
    Qty<"m"> x = 50.0*_("m");
    Qty<"m"> addendum_no_conversion(0.1);
    Qty<"cm"> addendum_conversion(10.0);

    auto y1=x+addendum_no_conversion;
    TEST_CHECK(y1.value() == 50.1);
    TEST_CHECK(y1.unit()[0]=='m');
    TEST_CHECK(y1.unit()[1]==0);

    auto y2=addendum_no_conversion+x;
    TEST_CHECK(y2.value() == 50.1);
    TEST_CHECK(y2.unit()[0]=='m');
    TEST_CHECK(y2.unit()[1]==0);

    auto y3=addendum_conversion+x;
    TEST_CHECK(y3.value() == 5010.0);
    TEST_CHECK(y3.unit()[0]=='c');
    TEST_CHECK(y3.unit()[1]=='m');
    TEST_CHECK(y3.unit()[2]==0);

    auto y4=x+addendum_conversion;
    TEST_CHECK(y4.value() == 50.1);
    TEST_CHECK(y4.unit()[0]=='m');
    TEST_CHECK(y4.unit()[1]==0);

}

void test_Qty_operators7() {
    // Comprehensive test of subtraction
    Qty<"m"> x = 50.0*_("m");
    Qty<"m"> addendum_no_conversion(0.1);
    Qty<"cm"> addendum_conversion(10.0);

    auto y1=x-addendum_no_conversion;
    TEST_CHECK(y1.value() == 49.9);
    TEST_CHECK(y1.unit()[0]=='m');
    TEST_CHECK(y1.unit()[1]==0);

    auto y2=addendum_no_conversion-x;
    TEST_CHECK(y2.value() == -49.9);
    TEST_CHECK(y2.unit()[0]=='m');
    TEST_CHECK(y2.unit()[1]==0);

    auto y3=addendum_conversion-x;
    TEST_CHECK(y3.value() == -4990.0);
    TEST_CHECK(y3.unit()[0]=='c');
    TEST_CHECK(y3.unit()[1]=='m');
    TEST_CHECK(y3.unit()[2]==0);

    auto y4=x-addendum_conversion;
    TEST_CHECK(y4.value() == 49.9);
    TEST_CHECK(y4.unit()[0]=='m');
    TEST_CHECK(y4.unit()[1]==0);

}

void test_Qty_operators8() {
    // Comprehensive test of multiplication by scalar
    Qty<"m"> x = 50.0*_("m");
    int multiplicand_int32 = -123;
    int64_t multiplicand_int64 = -123;
    double multiplicand_dbl = -123.0;

    auto y1=x*multiplicand_int32;
    TEST_CHECK(y1.value() == -6150.0);
    TEST_CHECK(y1.unit()[0]=='m');
    TEST_CHECK(y1.unit()[1]==0);

    auto y2=multiplicand_int32*x;
    TEST_CHECK(y2.value() == -6150.0);
    TEST_CHECK(y2.unit()[0]=='m');
    TEST_CHECK(y2.unit()[1]==0);

    auto y3=x*multiplicand_int64;
    TEST_CHECK(y3.value() == -6150.0);
    TEST_CHECK(y3.unit()[0]=='m');
    TEST_CHECK(y3.unit()[1]==0);

    auto y4=multiplicand_int64*x;
    TEST_CHECK(y4.value() == -6150.0);
    TEST_CHECK(y4.unit()[0]=='m');
    TEST_CHECK(y4.unit()[1]==0);

    auto y5=x*multiplicand_dbl;
    TEST_CHECK(y5.value() == -6150.0);
    TEST_CHECK(y5.unit()[0]=='m');
    TEST_CHECK(y5.unit()[1]==0);

    auto y6=multiplicand_dbl*x;
    TEST_CHECK(y6.value() == -6150.0);
    TEST_CHECK(y6.unit()[0]=='m');
    TEST_CHECK(y6.unit()[1]==0);
}

void test_Qty_operators9() {
    // Comprehensive test of division by scalar
    Qty<"m"> x = 50.0*_("m");
    int div_int32 = -123;
    int64_t div_int64 = -123;
    double div_dbl = -123.0;

    auto y1=x/div_int32;
    TEST_CHECK(y1.value() == -0.40650406504065040650406504065);
    TEST_CHECK(y1.unit()[0]=='m');
    TEST_CHECK(y1.unit()[1]==0);

    auto y2=div_int32/x;
    TEST_CHECK(y2.value() == -2.46);
    TEST_CHECK(y2.unit()[0]=='1');
    TEST_CHECK(y2.unit()[1]=='/');
    TEST_CHECK(y2.unit()[2]=='m');
    TEST_CHECK(y2.unit()[3]==0);

    auto y3=x/div_int64;
    TEST_CHECK(y3.value() == -0.40650406504065040650406504065);
    TEST_CHECK(y3.unit()[0]=='m');
    TEST_CHECK(y3.unit()[1]==0);

    auto y4=div_int64/x;
    TEST_CHECK(y4.value() == -2.46);
    TEST_CHECK(y4.unit()[0]=='1');
    TEST_CHECK(y4.unit()[1]=='/');
    TEST_CHECK(y4.unit()[2]=='m');
    TEST_CHECK(y4.unit()[3]==0);

    auto y5=x/div_dbl;
    TEST_CHECK(y5.value() == -0.40650406504065040650406504065);
    TEST_CHECK(y5.unit()[0]=='m');
    TEST_CHECK(y5.unit()[1]==0);

    auto y6=div_dbl/x;
    TEST_CHECK(y6.value() == -2.46);
    TEST_CHECK(y6.unit()[0]=='1');
    TEST_CHECK(y6.unit()[1]=='/');
    TEST_CHECK(y6.unit()[2]=='m');
    TEST_CHECK(y6.unit()[3]==0);

}

void test_Qty_operators10() {
    // Comprehensive test of multiplication and division of units with multiple tokens
    auto x = 50.0*_("kg*m/s^2");
    auto y = 10.0*_("m/s^2");

    auto y1=x/y;
    TEST_CHECK(y1.value() == 5.0);
    TEST_CHECK(y1.unit()[0]=='k');
    TEST_CHECK(y1.unit()[1]=='g');
    TEST_CHECK(y1.unit()[2]==0);

    auto y2=y*x;
    TEST_CHECK(y2.value() == 500.0);
    TEST_CHECK(y2.unit()[0]=='m');
    TEST_CHECK(y2.unit()[1]=='^');
    TEST_CHECK(y2.unit()[2]=='2');
    TEST_CHECK(y2.unit()[3]=='*');
    TEST_CHECK(y2.unit()[4]=='k');
    TEST_CHECK(y2.unit()[5]=='g');
    TEST_CHECK(y2.unit()[6]=='/');
    TEST_CHECK(y2.unit()[7]=='s');
    TEST_CHECK(y2.unit()[8]=='^');
    TEST_CHECK(y2.unit()[9]=='4');
    TEST_CHECK(y2.unit()[10]==0);

    auto y3=y/x;
    TEST_CHECK(y3.value() == 0.2);
    TEST_CHECK(y3.unit()[0]=='1');
    TEST_CHECK(y3.unit()[1]=='/');
    TEST_CHECK(y3.unit()[2]=='k');
    TEST_CHECK(y3.unit()[3]=='g');
    TEST_CHECK(y3.unit()[4]==0);

    auto y4=x*y;
    TEST_CHECK(y4.value() == 500.0);
    TEST_CHECK(y4.unit()[0]=='k');
    TEST_CHECK(y4.unit()[1]=='g');
    TEST_CHECK(y4.unit()[2]=='*');
    TEST_CHECK(y4.unit()[3]=='m');
    TEST_CHECK(y4.unit()[4]=='^');
    TEST_CHECK(y4.unit()[5]=='2');
    TEST_CHECK(y4.unit()[6]=='/');
    TEST_CHECK(y4.unit()[7]=='s');
    TEST_CHECK(y4.unit()[8]=='^');
    TEST_CHECK(y4.unit()[9]=='4');
    TEST_CHECK(y4.unit()[10]==0);

}

void test_Qty_operators11() {
    // Comprehensive test of multiplication and division of units with multiple tokens AND fractional exponents
    auto x = 50.0*_("kg*m^(1/3)/s^2");
    auto y = 10.0*_("m/s^1/2");

    auto y1=x/y;
    TEST_CHECK(y1.value() == 5.0);
    TEST_CHECK(y1.unit()[0]=='k');
    TEST_CHECK(y1.unit()[1]=='g');
    TEST_CHECK(y1.unit()[2]=='/');
    TEST_CHECK(y1.unit()[3]=='(');
    TEST_CHECK(y1.unit()[4]=='m');
    TEST_CHECK(y1.unit()[5]=='^');
    TEST_CHECK(y1.unit()[6]=='2');
    TEST_CHECK(y1.unit()[7]=='/');
    TEST_CHECK(y1.unit()[8]=='3');
    TEST_CHECK(y1.unit()[9]=='*');
    TEST_CHECK(y1.unit()[10]=='s');
    TEST_CHECK(y1.unit()[11]=='^');
    TEST_CHECK(y1.unit()[12]=='3');
    TEST_CHECK(y1.unit()[13]=='/');
    TEST_CHECK(y1.unit()[14]=='2');
    TEST_CHECK(y1.unit()[15]==')');
    TEST_CHECK(y1.unit()[16]==0);

    auto y2=y*x;
    TEST_CHECK(y2.value() == 500.0);
    TEST_CHECK(y2.unit()[0]=='m');
    TEST_CHECK(y2.unit()[1]=='^');
    TEST_CHECK(y2.unit()[2]=='4');
    TEST_CHECK(y2.unit()[3]=='/');
    TEST_CHECK(y2.unit()[4]=='3');
    TEST_CHECK(y2.unit()[5]=='*');
    TEST_CHECK(y2.unit()[6]=='k');
    TEST_CHECK(y2.unit()[7]=='g');
    TEST_CHECK(y2.unit()[8]=='/');
    TEST_CHECK(y2.unit()[9]=='s');
    TEST_CHECK(y2.unit()[10]=='^');
    TEST_CHECK(y2.unit()[11]=='5');
    TEST_CHECK(y2.unit()[12]=='/');
    TEST_CHECK(y2.unit()[13]=='2');
    TEST_CHECK(y2.unit()[14]==0);

    auto y3=y/x;
    TEST_CHECK(y3.value() == 0.2);
    TEST_CHECK(y3.unit()[0]=='m');
    TEST_CHECK(y3.unit()[1]=='^');
    TEST_CHECK(y3.unit()[2]=='2');
    TEST_CHECK(y3.unit()[3]=='/');
    TEST_CHECK(y3.unit()[4]=='3');
    TEST_CHECK(y3.unit()[5]=='*');
    TEST_CHECK(y3.unit()[6]=='s');
    TEST_CHECK(y3.unit()[7]=='^');
    TEST_CHECK(y3.unit()[8]=='3');
    TEST_CHECK(y3.unit()[9]=='/');
    TEST_CHECK(y3.unit()[10]=='2');
    TEST_CHECK(y3.unit()[11]=='/');
    TEST_CHECK(y3.unit()[12]=='k');
    TEST_CHECK(y3.unit()[13]=='g');
    TEST_CHECK(y3.unit()[14]==0);

    auto y4=x*y;
    TEST_CHECK(y4.value() == 500.0);
    TEST_CHECK(y4.unit()[0]=='k');
    TEST_CHECK(y4.unit()[1]=='g');
    TEST_CHECK(y4.unit()[2]=='*');
    TEST_CHECK(y4.unit()[3]=='m');
    TEST_CHECK(y4.unit()[4]=='^');
    TEST_CHECK(y4.unit()[5]=='4');
    TEST_CHECK(y4.unit()[6]=='/');
    TEST_CHECK(y4.unit()[7]=='3');
    TEST_CHECK(y4.unit()[8]=='/');
    TEST_CHECK(y4.unit()[9]=='s');
    TEST_CHECK(y4.unit()[10]=='^');
    TEST_CHECK(y4.unit()[11]=='5');
    TEST_CHECK(y4.unit()[12]=='/');
    TEST_CHECK(y4.unit()[13]=='2');
    TEST_CHECK(y4.unit()[14]==0);
}

void test_Qty_operators12() {
    // Comprehensive test of pow operation on unit
    auto x = 50.0*_("kg*m^(1/3)/s^2");

    // Exponents as templates as they are known at compile time
    auto y1=pow<2>(x);
    TEST_CHECK(y1.value() == 2500.0);
    TEST_CHECK(y1.unit()[0]=='k');
    TEST_CHECK(y1.unit()[1]=='g');
    TEST_CHECK(y1.unit()[2]=='^');
    TEST_CHECK(y1.unit()[3]=='2');
    TEST_CHECK(y1.unit()[4]=='*');
    TEST_CHECK(y1.unit()[5]=='m');
    TEST_CHECK(y1.unit()[6]=='^');
    TEST_CHECK(y1.unit()[7]=='2');
    TEST_CHECK(y1.unit()[8]=='/');
    TEST_CHECK(y1.unit()[9]=='3');
    TEST_CHECK(y1.unit()[10]=='/');
    TEST_CHECK(y1.unit()[11]=='s');
    TEST_CHECK(y1.unit()[12]=='^');
    TEST_CHECK(y1.unit()[13]=='4');
    TEST_CHECK(y1.unit()[14]==0);

    auto y2=pow<-2>(x);
    TEST_CHECK(y2.value() == 1.0/2500.0);
    TEST_CHECK(y2.unit()[0]=='s');
    TEST_CHECK(y2.unit()[1]=='^');
    TEST_CHECK(y2.unit()[2]=='4');
    TEST_CHECK(y2.unit()[3]=='/');
    TEST_CHECK(y2.unit()[4]=='(');
    TEST_CHECK(y2.unit()[5]=='k');
    TEST_CHECK(y2.unit()[6]=='g');
    TEST_CHECK(y2.unit()[7]=='^');
    TEST_CHECK(y2.unit()[8]=='2');
    TEST_CHECK(y2.unit()[9]=='*');
    TEST_CHECK(y2.unit()[10]=='m');
    TEST_CHECK(y2.unit()[11]=='^');
    TEST_CHECK(y2.unit()[12]=='2');
    TEST_CHECK(y2.unit()[13]=='/');
    TEST_CHECK(y2.unit()[14]=='3');
    TEST_CHECK(y2.unit()[15]==')');
    TEST_CHECK(y2.unit()[16]==0);


    auto y3=pow<1,2>(x);
    TEST_CHECK(y3.value() == std::sqrt(50.0));
    TEST_CHECK(y3.unit()[0]=='k');
    TEST_CHECK(y3.unit()[1]=='g');
    TEST_CHECK(y3.unit()[2]=='^');
    TEST_CHECK(y3.unit()[3]=='1');
    TEST_CHECK(y3.unit()[4]=='/');
    TEST_CHECK(y3.unit()[5]=='2');
    TEST_CHECK(y3.unit()[6]=='*');
    TEST_CHECK(y3.unit()[7]=='m');
    TEST_CHECK(y3.unit()[8]=='^');
    TEST_CHECK(y3.unit()[9]=='1');
    TEST_CHECK(y3.unit()[10]=='/');
    TEST_CHECK(y3.unit()[11]=='6');
    TEST_CHECK(y3.unit()[12]=='/');
    TEST_CHECK(y3.unit()[13]=='s');
    TEST_CHECK(y3.unit()[14]==0);

    auto y4=sqrt(x);
    TEST_CHECK(y4.value() == std::sqrt(50.0));
    TEST_CHECK(y4.unit()[0]=='k');
    TEST_CHECK(y4.unit()[1]=='g');
    TEST_CHECK(y4.unit()[2]=='^');
    TEST_CHECK(y4.unit()[3]=='1');
    TEST_CHECK(y4.unit()[4]=='/');
    TEST_CHECK(y4.unit()[5]=='2');
    TEST_CHECK(y4.unit()[6]=='*');
    TEST_CHECK(y4.unit()[7]=='m');
    TEST_CHECK(y4.unit()[8]=='^');
    TEST_CHECK(y4.unit()[9]=='1');
    TEST_CHECK(y4.unit()[10]=='/');
    TEST_CHECK(y4.unit()[11]=='6');
    TEST_CHECK(y4.unit()[12]=='/');
    TEST_CHECK(y4.unit()[13]=='s');
    TEST_CHECK(y4.unit()[14]==0);

}

void test_Qty_operators13() {
    // Comprehensive test of pow operation on units with numeric part (conversions) and exponents
    auto x = 16*_("3/4_apples");

    auto y1=pow<2>(x);
    TEST_CHECK(y1.value() == 256.0);
    TEST_CHECK(y1.unit()[0]=='9');
    TEST_CHECK(y1.unit()[1]=='/');
    TEST_CHECK(y1.unit()[2]=='1');
    TEST_CHECK(y1.unit()[3]=='6');
    TEST_CHECK(y1.unit()[4]=='_');
    TEST_CHECK(y1.unit()[5]=='a');
    TEST_CHECK(y1.unit()[6]=='p');
    TEST_CHECK(y1.unit()[7]=='p');
    TEST_CHECK(y1.unit()[8]=='l');
    TEST_CHECK(y1.unit()[9]=='e');
    TEST_CHECK(y1.unit()[10]=='s');
    TEST_CHECK(y1.unit()[11]=='^');
    TEST_CHECK(y1.unit()[12]=='2');
    TEST_CHECK(y1.unit()[13]==0);

    // Push the digits to the limit
    Qty<"3_psi"> z = 1.234*_("3_kPa");

    auto y2=pow<1,2>(z);
    TEST_CHECK(std::abs(y2.value() - 0.42305622363827502437298)<1e-15);

    const char expected[]="3900231685776981/2251799813685248_psi^1/2";
    auto unit = y2.unit();
    for(auto i=0;expected[i]!=0;++i) {
        TEST_CHECK(unit[i]==expected[i]);
    }

}




/*
void other_function() {
    Qty<"m"> x = 50.0*_("m");

    auto y=x+(10.0*_("cm"));
    // Try to create a variable (not constexpr)b
    auto density_whatever = 4.5*_("kg/ft^3");

    // Assign it to a variable with a different unit, the compiler
    // should emit code that does the conversion at run time
    // simply by multiplying the conversion factor, all units stuff
    // is done at compile time and no code should be generated
    // even though these two aren't constexpr values
    Qty<"kg/m^3"> density = density_whatever;

    std::cout << "Density = " << density.value() << "_" << density.unit() << std::endl;

    // Do an assignment of a temporary, it should convert the units correctly
    // then emit code that only stores the result (even the conversion factor
    // should be applied at compile time)
    Qty<"m/s^2"> g=32.174*_("ft/s^2");

    std::cout << "g = " << g.value() << "_" << g.unit() << std::endl;

    // Make another variable
    Qty<"m^2"> area=100.0*_("m^2");
    // And multiply them together with the length from before, the compiler
    // should generate code that multiplies the values together, the unit stuff
    // is all done at compile time
    auto volume = y * area * g;

    //std::cout << "Areag = " << areag.value << "_" << areag.unit.u_def << std::endl;
    std::cout << "Volume = " << volume.value() << "_" << volume.unit() << std::endl;

    y+=7*_("cm");
    y-=1*_("mm");

    std::cout << "y=" << y.value() << "_" << y.unit() << std::endl;

    Qty<"3_apples"> trio{1.0};

    auto fourapples = trio + 1*_("apples");

    std::cout << "4apples=" << fourapples.value() << "_" << fourapples.unit() << std::endl;

    // ************************************************************
    // Basic tests for the run-time components

    // Constructor
    RQty velocity(10.0,"m/s");

    // Conversion from Qty<U>
    RQty speed = 10.0*_("m/s");

    // Multiplication of units at run time
    speed*=velocity;

    std::cout << "speed^2 =" << speed.value() << "_" << speed.unit() << std::endl;

    // Incompatibility of units at run time
    try {
    speed += velocity;
    }
    catch(std::exception& e) {
        std::cout << "Exception thrown: " << e.what() << std::endl;
    }

    speed= velocity / 2;

    std::cout << "speed/2 =" << speed.value() << "_" << speed.unit() << std::endl;

    auto flow = massFlow(density,area,velocity);

    std::cout << "flow=" << flow.value() << "_" << flow.unit() << std::endl;

    // Check if the constants-as-units work

    auto angle = 180*_("°");

    std::cout << "cos 180 degrees = " << cosUnit(angle) << std::endl;

    // Test some greek letters

    const RQty length(1,"µ");

    Qty<"Å"> angstrom = length;

    std::cout << "length in Å = " << angstrom.value() << "_Å" << std::endl;
}
*/
TEST_LIST = {
    {"BasicMath-Pow",test_basicmath1},
    {"BasicMath-nRoot",test_basicmath2},
    {"BasicMath-Conversion",test_basicmath3},

    {"UnitDefinition-CanParse",test_UnitDefinition_parser},
    {"UnitDefinition-ParseNumber", test_UnitDefinition_parser_number},
    {"UnitDefinition-ParseNumber2", test_UnitDefinition_parser_number2},
    {"UnitDefinition-ParseNumber3", test_UnitDefinition_parser_number3},
    {"UnitDefinition-ParseNumber4", test_UnitDefinition_parser_number4},
    {"UnitDefinition-ParseNumber5", test_UnitDefinition_parser_number5},
    {"UnitDefinition-ParseNumber6", test_UnitDefinition_parser_number6},
    {"UnitDefinition-ParseNumber7", test_UnitDefinition_parser_number7},
    {"UnitDefinition-ParseNumber8", test_UnitDefinition_parser_number8},
    {"UnitDefinition-ParseNumber9", test_UnitDefinition_parser_number9},
    {"UnitDefinition-ParseNumber10", test_UnitDefinition_parser_number10},
    {"UnitDefinition-ParseNumber11", test_UnitDefinition_parser_number11},
    {"UnitDefinition-ParseNumber12", test_UnitDefinition_parser_number12},
    {"UnitDefinition-ParseNumber13", test_UnitDefinition_parser_number13},
    {"UnitDefinition-ParseNumber14", test_UnitDefinition_parser_number14},
    {"UnitDefinition-ParseNumber15", test_UnitDefinition_parser_number15},
    {"UnitDefinition-ParseNumber16", test_UnitDefinition_parser_number16},
    {"UnitDefinition-ParseNumber17", test_UnitDefinition_parser_number17},
    {"UnitDefinition-ParseNumber18", test_UnitDefinition_parser_number18},
    {"UnitDefinition-ParseNumber19", test_UnitDefinition_parser_number19},
    {"UnitDefinition-ParseNumber20", test_UnitDefinition_parser_number20},
    {"UnitDefinition-ParseExponent1", test_UnitDefinition_parser_unitExponent1},
    {"UnitDefinition-ParseExponent2", test_UnitDefinition_parser_unitExponent2},
    {"UnitDefinition-ParseExponent3", test_UnitDefinition_parser_unitExponent3},
    {"UnitDefinition-ParseExponent4", test_UnitDefinition_parser_unitExponent4},
    {"UnitDefinition-ParseExponent5", test_UnitDefinition_parser_unitExponent5},
    {"UnitDefinition-ParseExponent6", test_UnitDefinition_parser_unitExponent6},
    {"UnitDefinition-ParseExponent7", test_UnitDefinition_parser_unitExponent7},
    {"UnitDefinition-ParseExponent8", test_UnitDefinition_parser_unitExponent8},
    {"UnitDefinition-ParseExponent9", test_UnitDefinition_parser_unitExponent9},
    {"UnitDefinition-ParseExponent10", test_UnitDefinition_parser_unitExponent10},
    {"UnitDefinition-ParseExponent11", test_UnitDefinition_parser_unitExponent11},
    {"UnitDefinition-ParseExponent12", test_UnitDefinition_parser_unitExponent12},
    {"UnitDefinition-ParseExponent13", test_UnitDefinition_parser_unitExponent13},
    {"UnitDefinition-ParseExponent14", test_UnitDefinition_parser_unitExponent14},
    {"UnitDefinition-ParseExponent15", test_UnitDefinition_parser_unitExponent15},
    {"UnitDefinition-ParseExponent16", test_UnitDefinition_parser_unitExponent16},
    {"UnitDefinition-ParseExponent17", test_UnitDefinition_parser_unitExponent17},
    {"UnitDefinition-ParseExponent18", test_UnitDefinition_parser_unitExponent18},
    {"UnitDefinition-ParseExponent19", test_UnitDefinition_parser_unitExponent19},
    {"UnitDefinition-ParseExponent20", test_UnitDefinition_parser_unitExponent20},
    {"UnitDefinition-ParseExponent21", test_UnitDefinition_parser_unitExponent21},
    {"UnitDefinition-MultipleTokens1", test_UnitDefinition_parser_multipleTokens1},
    {"UnitDefinition-MultipleTokens2", test_UnitDefinition_parser_multipleTokens2},
    {"UnitDefinition-MultipleTokens3", test_UnitDefinition_parser_multipleTokens3},
    {"UnitDefinition-MultipleTokens4", test_UnitDefinition_parser_multipleTokens4},
    {"UnitDefinition-MultipleTokens5", test_UnitDefinition_parser_multipleTokens5},
    {"UnitDefinition-MultipleTokens6", test_UnitDefinition_parser_multipleTokens6},
    {"UnitDefinition-MultipleTokens7", test_UnitDefinition_parser_multipleTokens7},
    {"UnitDefinition-MultipleTokens8", test_UnitDefinition_parser_multipleTokens8},
    {"UnitDefinition-MultipleTokens9", test_UnitDefinition_parser_multipleTokens9},
    {"UnitDefinition-MultipleTokens10", test_UnitDefinition_parser_multipleTokens10},
    {"UnitDefinition-MultipleTokens11", test_UnitDefinition_parser_multipleTokens11},
    {"UnitDefinition-Update1", test_UnitDefinition_update1},
    {"UnitDefinition-Update2", test_UnitDefinition_update2},
    {"UnitDefinition-Update3", test_UnitDefinition_update3},
    {"UnitDefinition-Update4", test_UnitDefinition_update4},
    {"UnitDefinition-Update5", test_UnitDefinition_update5},

    {"Qty-Operators1",test_Qty_operators1},
    {"Qty-Operators2",test_Qty_operators2},
    {"Qty-Operators3",test_Qty_operators3},
    {"Qty-Operators4",test_Qty_operators4},
    {"Qty-Operators5",test_Qty_operators5},
    {"Qty-Operators6",test_Qty_operators6},
    {"Qty-Operators7",test_Qty_operators7},
    {"Qty-Operators8",test_Qty_operators8},
    {"Qty-Operators9",test_Qty_operators9},
    {"Qty-Operators10",test_Qty_operators10},
    {"Qty-Operators11",test_Qty_operators11},
    {"Qty-Operators12",test_Qty_operators12},
    {"Qty-Operators13",test_Qty_operators13},
    {NULL,NULL}
};
