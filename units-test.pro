TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

HEADERS += \
        allunits.hpp \
        fraction_units.hpp

CONFIG(debug, debug|release) {
QMAKE_CXXFLAGS += -g -O0 -Wpedantic -Wshadow -std=gnu++20
}
CONFIG(release, debug|release) {
QMAKE_CXXFLAGS += -g -O3 -DNDEBUG -Wpedantic -std=gnu++20
}
