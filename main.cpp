#include <iostream>
#include <cmath>

#include "fraction_units.hpp"

#include "external/test/include/acutest.h"

auto massFlow(const Qty<"kg/m^3"> density, const Qty<"m^2"> area, const Qty<"m/s"> velocity) {
    return density * area * velocity;
}

double cosUnit(const Qty<"r"> angle) {
    return std::cos(angle.value());
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

    TEST_CHECK(check.error_state==UnitError::NoError);
    TEST_CHECK(check.definition[0].tokStart==0);
    TEST_CHECK(check.definition[0].tokEnd==0);

    TEST_CHECK(check.value_ip==12345);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-4);
}

void test_UnitDefinition_parser_number7()
{
    // Parse a number, strange character
    UnitDefinition check("check","1.23#45");

    TEST_CHECK(check.error_state==UnitError::InvalidToken);
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
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}

void test_UnitDefinition_parser_number12()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23eE2");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}

void test_UnitDefinition_parser_number13()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23e-2.5");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}

void test_UnitDefinition_parser_number14()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","e-2.5");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}

void test_UnitDefinition_parser_number15()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","/e-2.5");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
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
    // Should be a unit 'e' multiplied by unit 'm'
    UnitDefinition check("check","1.23e_m");
    TEST_CHECK(check.error_state==UnitError::NoError);

    TEST_CHECK(check.value_ip==123);
    TEST_CHECK(check.value_den==1);
    TEST_CHECK(check.value_exp==-2);

    // 'e'
    TEST_CHECK(check.definition[0].tokStart==4);
    TEST_CHECK(check.definition[0].tokEnd==5);
    TEST_CHECK(check.definition[0].expNum==1);
    TEST_CHECK(check.definition[0].expDen==1);
    // 'm'
    TEST_CHECK(check.definition[1].tokStart==6);
    TEST_CHECK(check.definition[1].tokEnd==7);
    TEST_CHECK(check.definition[1].expNum==1);
    TEST_CHECK(check.definition[1].expDen==1);
    // End
    TEST_CHECK(check.definition[2].tokStart==0);
    TEST_CHECK(check.definition[2].tokEnd==0);

}

void test_UnitDefinition_parser_number18()
{
    // Parse a number, invalid exponent
    UnitDefinition check("check","1.23e/567");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}

void test_UnitDefinition_parser_number19()
{
    // Expressions not allowed by themselves in the middle of a unit
    UnitDefinition check("check","1.23*4");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}

void test_UnitDefinition_parser_number20()
{
    // Expressions not allowed by themselves in the middle of a unit
    UnitDefinition check("check","1.234(5)");
    TEST_CHECK(check.error_state==UnitError::InvalidToken);
}



void other_function() {
    // Try to break the parser with some broken expressions
    UnitDefinition meter("m","1.2345_kg/m^  3    ( / s^3)^2/3");

    std::cout << meter.u_name << "=" << meter.u_def << std::endl;
    std::cout << "Error state = " << UnitErrorMessages[meter.error_state] << std::endl;
    std::cout << "value=" << meter.value_ip << "/" << meter.value_den << " x 10^" << meter.value_exp << std::endl;

    for(size_t i=0;i<maxTokens;++i) {
        if(meter.definition[i].expDen==0)
            break;
        std::cout << "'" << std::string_view(meter.u_def+meter.definition[i].tokStart,meter.u_def+meter.definition[i].tokEnd) << "'^ " << meter.definition[i].expNum << "/" << meter.definition[i].expDen << std::endl;
    }

    // Try to use some quantities, this is all constexpr so verify the compiler
    // generates code that simply stores the number and nothing else
    // First test just test defining a constexpr with assignment of the same unit
    Qty<"m"> x = 50.0*_("m");

    // Try an addition of 2 constexpr values, the compiler should again
    // only emit code that stores the result. The unit of the result
    // is by convention the unit of the left operand, so 'm'
    // and the cm should be automatically converted at compile time
    auto y=x+(10.0*_("cm"));

    std::cout << "y=" << y.value() << "_" << y.unit() << std::endl;

    // Try to create a variable (not constexpr)
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

TEST_LIST = {
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
    {NULL,NULL}
};
