#include <iostream>
#include <cmath>

#include "fraction_units.hpp"

auto massFlow(const Qty<"kg/m^3"> density, const Qty<"m^2"> area, const Qty<"m/s"> velocity) {
    return density * area * velocity;
}

double cosUnit(const Qty<"r"> angle) {
    return std::cos(angle.value());
}

int main()
{
    // Try creating a unit, then print all tokens and exponents
    UnitDefinition check("check","kg/ft^3");

    std::cout << "Check = " << std::endl;

    for(size_t i=0;i<maxTokens;++i) {
        if(check.definition[i].expDen==0)
            break;
        std::cout << "'" << std::string_view(check.u_def+check.definition[i].tokStart,check.u_def+check.definition[i].tokEnd) << "'^ " << check.definition[i].expNum << "/" << check.definition[i].expDen << std::endl;
    }

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
    return 0;
}
