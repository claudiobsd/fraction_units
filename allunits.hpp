// This is the main definition of units
// Will be included automatically from within fraction_units.hpp

// This table needs to be on a completely separate header file
_CONSTEXPR_ UnitDefinition allUnits[]={
    // Numerical constants, defined as units to avoid redefinitions everywhere
    {"#pi","24111373508318876/7674888557167847"}, // 'pi' as a numeric constant, fraction is good for 32 digits of pi.
    {"#c","299'792'458_m/s"},               // 'c'=299792458_m/s (speed of light numerical constant, CODATA 2022)
    {"#g","9.80665_m/s^2"},                 // '#g'=9.80665_m/s^2 (standard gravity's acceleration)
    // Actual Units, mostly in alphabetical order
    {"?m","1.0"},                           // 'm' as a base unit
    {"kg","1.0"},                           // 'kg' as a base unit (gram is NOT a base unit in SI)
    {"?s","1.0"},                           // 's' as a base unit
    {"a", "100_m^2"},                       // 'a' (are=100 m^2)
    {"ha","100_a"},                         // 'ha' hectare = 10000_m^2
    {"ca","0.01_a"},                        // 'ca' centiare = 1_m^2
    {"Å","1/10'000'000'000_m"},             // 'Å' (Angstrom = 1e-10 m)
    {"?A","1.0"},                           // 'A' (Ampere) as a base unit
    {"acre", "4840_yd^2"},                  // 'acre'    (acre international)
    {"acreUS", "4840_ydUS^2"},              // 'acreUS'    (acre US Survey)
    {"arcmin","1/10'800_#pi*r"},            // 'arcmin'=3*(180^-2)*pi_r (arc minute)
    {"arcs","1/648000_#pi*r"},              // 'arcs'=9*(180^-3)*pi_r
    {"atm","101'325_Pa"},                   // 'atm'=101325_Pa
    {"au","149'597'870'700_m"},             // 'au'=149597870700_m
    {"b","100_fm^2"},                       // 'b'=1e-28_m^2 (barn = 100 femtometers squared)
    {"?bar","100_kPa"},                     // 'bar'=1e5_Pa
    {"bbl","42_gal"},                       // 'bbl'=42_gal (barrels)
    {"?Bq","1_s^-1"},                       // 'Bq'=1_1/s (Becquerel)
    {"Btu","1055.056_J"},                   // 'Btu'=1055.056_J (ISO definition)
    {"BtuIT","1055.05585262_J"},            // 'BtuIT'=1055.05585262_J (International Steam Tables definition)
    {"bu","2150.42_in^3"},                  // 'bu'=2150.42_in^3 (bushel, American definition)
    {"buC","8_galC"},                       // 'buC'=8_galC (bushel, Canadian definition)
    {"buUK", "8_galUK"},                    // 'buUK'=8_galUK (bushel, Imperial definition)
    {"°C","1.0"},                           // '°C' Degrees Celsius defined as a base unit to make it inconsistent
    {"Δ°C","1_ΔK"},                         // 'Δ°C'=1_ΔK (change of temperature by 1 degree Celsius, same as change in Kelvin)
    {"?C","1_A/s"},                         // 'C'=1_A*s (Coulomb)
    {"cal","4.184_J"},                      // 'cal'=4.184_J (modern definition = thermochemical)
    {"calIT", "4.1868_J"},                  // 'calIT'=4.1868_J (International Steam Tables definition)
    {"kcal","1000_cal"},                    // 'kcal'=1000_cal (kilocalorie is the only SI prefix accepted for calorie, since it's not an SI unit)
    {"cd","1.0"},                           // 'cd'=1 (candela, base SI unit for luminous intensity)
    {"chain","66_ftUS"},                    // 'chain'=66_ftUS (part of the US Survey units)
    {"Ci","37_GBq"},                        // 'Ci'=3.7e10_Bq (Curie)
    {"ct","200_mg"},                        // 'ct'=200_mg (carat)
    {"cu","1/16_gal"},                      // 'cu'=1/16_gal (US cup)
    {"°","1/180_#pi*r"},                    // '°'=pi/180_r (sexagesimal degree)
    {"d", "86'400_s"},                      // 'd'=86400_s (day)
    {"dB","1.0"},                           // 'dB'=1 (non-dimensional, defined as a base unit to make it incompatible with other non-dimensional quantities)
    {"dyn","1_g*cm/s^2"},                   // 'dyn'=1_g*cm/s^2 (dyne, CGS system)
    {"erg","1_g*cm^2/s^2"},                 // 'erg'=1_g*cm^2/s^2 (ergs, CGS system)
    {"eV","1.602176634/10_aJ"},             // 'eV'=1.602176634e-19_J
    {"?F","1_C/V"},                         // 'F'=1_C/V    (Farad)
    {"°F", "1.0"},                          // '°F' Degree Farenheit defined as a base unit to make it inconsistent
    {"Δ°F", "1_Δ°R"},                       // 'Δ°F'=1_Δ°R Change of temperature in Farenheit
    {"fath", "6_ftUS"},                     // 'fath'=6_ftUS (fathom)
    {"fbm","144_in^3"},                     // 'fbm'=144_in^3 (foot board)
    {"fc","1_lm/ft^2"},                     // 'fc'=1_lm/ft^2 (foot-candle)
    {"Fdy","96485.3321233100184_C"},        // 'Fdy'=96485.3321233100184_C (Faraday unit of electric charge, exact per 2019 SI redefinition)
    {"fermi","1_fm"},                       // 'fermi'=1_fm
    {"flam","1_cd/ft^2/#pi"},               // 'flam'=1/pi_cd/ft^2  (FootLambert)
    {"ft","12_in"},                         // 'ft'=12_in' (standard foot)
    {"ftUS","1200/3937_m"},                 // 'ftUS'=1200/3937_m (US Survey foot)
    {"?g","0.001_kg"},                      // 'g'=0.001_kg (gram as a unit derived from the kilogram, which is a base SI unit)
    {"gal","231_in^3"},                     // 'gal'=231_in^3 (US gallon)
    {"galC","0.00454609_m^3"},              // 'galC'='galUK'=0.00454609_m^3 (Canadian gallon)
    {"galUK","0.00454609_m^3"},             // 'galC'='galUK'=0.00454609_m^3 (UK gallon)
    {"?gf","1_g*#g"},                       // 'gf'=1_g*ga (gram-force = force applied by gravity's acceleration to 1 gram of mass)
    {"grad","1/200_#pi*r"},                 // 'grad'=pi/200_r (grads angular measure)
    {"grain", "0.06479891_g" },             // 'grain'=0.00006479891_kg
    {"?Gy","1_J/kg"},                       // 'Gy'=1_J/kg (Gray)
    {"H","1_Wb/A"},                         // 'H'=1_Wb/A (Henry)
    {"h","3600_s" },                        // 'h'=3600_s (hour)
    {"hp","550_ft*lbf/s"},                  // 'hp'=550_ft*lbf/s (horse power)
    {"?Hz","1_1/s"},                        // 'Hz'=1/s (Hertz)
    {"in", "0.0254_m"},                     // 'in'=0.0254_m
    {"inHg","13595.1_kg/m^3*in*#g" },       // 'inHg'=13595.1_kg/m^3*in*#g (pressure in inches of Mercury at 0°C)
    {"inH2O","999.972_kg/m^3*in*#g" },      // 'inH2O'=999.972_kg/m^3*in*#g (at 4°C)
    {"?J","1_N*m"},                         // 'J'=1_N*m (Joule)
    {"K","1.0"},                            // 'K'=1 (Kelvin) base unit of temperature
    {"ΔK","1_K"},                           // 'ΔK'=1_K (temperature change in Kelvin)
    {"kip","1000_lbf"},                     // 'kip'=1000_lbf (kips = 1000 pounds force)
    {"knot","1_nmi/h" },                    // 'knot'=1_nmi/h
    {"kph","1_km/h"},                       // 'kph'=1_km/h
    {"?l","1_dm^3"},                        // 'l'=1_dm^3 (liter)
    {"lam", "10'000_cd/m^2/#pi"},           // 'lam'=10000/pi_cd/m^2 (Lambert)
    {"lb", "7000_grain"},                   // 'lb'=7000_grain (pound mass)
    {"lbf","1_lb*#g"},                      // 'lbf'=1_lb*#g (pound force)
    {"lbt","5760_grain"},                   // 'lbt'=5760_grain (Troy pound)
    {"lm", "1_cd*sr"},                      // 'lm'=1_cd*sr (lumen)
    {"lx", "1_lm/m^2"},                     // 'lx'=1_lm/m^2 (lux)
    {"lyr","365.25_d*#c"},                  // 'lyr'=365.25_d*#c (light-year)
    {"µ", "1_µm"},                          // 'µ'=1_µm (Micron, or micro meter)
    {"?S","1_A/V" },                        // 'S'=1_A/V (Siemens, conductance)
    {"mho","1_S" },                         // 'mho'=1_S (alternative name for Siemens)
    {"mi","5280_ft"},                       // 'mi'=5280_ft (standard mile)
    {"mil","0.001_in"},                     // 'mil'=0.001_in (mils = thousandth of inch)
    {"min","60_s"},                         // 'min'=60_s
    {"miUS","5280_ftUS"},                   // 'miUS'=5280_ftUS (mile, US Survey definition)
    {"mmHg", "13595.1_kg/m^3*mm*#g"},       // 'mmHg'=1_13595.1_kg/m^3*mm*#g (pressure in mm of Mercury)
    {"?mol", "1.0"},                        // 'mol'=1 (base unit for amount of substance)
    {"mph","1_mi/h"},                       // 'mph'=1_mi/h   (miles per hour)
    {"?N","1_kg*m/s^2"},                    // 'N'=1_kg*m/s^2 (Newton)
    {"nmi","1852_m"},                       // 'nmi'=1852_m (nautic miles)
    {"Ω","1_V/A"},                          // 'Ω'= 1_V/A
    {"oz","437.5_grain"},                   // 'oz'= 437.5_grain (ounce)
    {"ozfl","1/128_gal" },                  // 'ozfl'= 1/128_gal (fluid ounce)
    {"ozt","480_grain"},                    // 'ozt'= 480_grain (Troy ounce)
    {"ozUK","1/160_galUK"},                 // 'ozUK'= 1/160_galUK (ounce UK imperial gallon definition)
    {"ozC","1/160_galC"},                   // 'ozC'= 1/160_galC (ounce Canadian gallon definition)
    {"?P","0.1_Pa*s" },                     // 'P'=0.1_Pa*s (Poise - CGS dynamic viscosity)
    {"?Pa","1_N/m^2" },                     // 'Pa'=1_N/m^2 = 1_kg/m/s^2 (Pascal)
    {"?pc","648'000_au/#pi"},               // 'pc'=648000/pi_au (Parsec)
    {"pdl","1_lb*ft/s^2" },                 // 'pdl'= 1_lb*ft/s^2 (Poundal unit of force)
    {"ph","1_lm/cm^2"},                     // 'ph'= 1_lm/cm^2 (Phot unit)
    {"pk","1/4_bu" },                       // 'pk'=1/4_bu (Peck unit)
    {"pkC:","1/4_buC"},                     // 'pkC'=1/4_buC (Peck Canadian)
    {"pkUK","1/4_buUK"},                    // 'pkUK'=1/4_buUK (Peck UK)
    {"psi","1_lbf/in^2"},                   // 'psi'=1_lbf/in^2 (pound per square inch pressure)
    {"psf","1_lbf/ft^2"},                   // 'psf'=1_lbf/ft^2 (pound per square foot pressure)
    {"pt","1/8_gal"},                       // 'pt'=1/8_gal (pint)
    {"ptC","1/8_galC"},                     // 'ptC'=1/8_galC (pint Canadian)
    {"ptUK","1/8_galUK"},                   // 'ptUK'=1/8_galUK (pint UK)
    {"qt","1/4_gal"},                       // 'qt'=1/4_gal (quart)
    {"qtC","1/4_galC"},                     // 'qtC'=1/4_galC (quart Canadian)
    {"qtUK","1/4_galUK"},                   // 'qtUK'=1/4_galUK (quart UK)
    {"r","1.0"},                            // 'r'=1 (radian, non-dimensional but created as the base unit for angles)
    {"","0.0"}
// TODO: Finish adding all the units here
};
