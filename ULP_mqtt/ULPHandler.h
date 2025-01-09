
// ULPHandler.h
#ifndef ULPHANDLER_H
#define ULPHANDLER_H

#define ULP_INIT_MARKER 0x1234

#include <Arduino.h> //for serial

namespace ULPHandler {
    void setupULP();
    bool isULPInitialized();
    uint32_t getCount();
    float getRate();
    void enter_deep_sleep();
}

#endif
