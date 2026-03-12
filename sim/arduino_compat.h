/**
 * sim/arduino_compat.h — Stubs Arduino-specific APIs for desktop simulator.
 * Include this instead of Arduino.h in files that need millis(), Serial, etc.
 * Only compiled when SIMULATOR=1 is defined.
 */
#pragma once

#ifdef SIMULATOR

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// millis() — milliseconds since program start
static inline uint32_t millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

static inline void delay(uint32_t ms) {
    struct timespec ts = { (time_t)(ms / 1000), (long)((ms % 1000) * 1000000L) };
    nanosleep(&ts, NULL);
}

// Serial stub — redirect to printf
struct SerialStub {
    static void begin(int) {}
    static void println(const char *s)           { printf("%s\n", s); }
    static void print(const char *s)             { printf("%s", s);   }
    static void printf(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
};
static SerialStub Serial;

#endif // SIMULATOR
