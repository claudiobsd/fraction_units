TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

HEADERS += \
        allunits.hpp \
        fraction_units.hpp

CONFIG(debug, debug|release) {
QMAKE_CXXFLAGS += -Og
}
CONFIG(release, debug|release) {
QMAKE_CXXFLAGS += -O3 -DNDEBUG
}
