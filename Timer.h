#pragma once

#include <windows.h>

struct Timer {
    Timer() {
        QueryPerformanceFrequency(&frequency);
    }

    void start() {
        QueryPerformanceCounter(&begin);
    }

    void stop() {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        elapsed.QuadPart = (1000000 * (end.QuadPart - begin.QuadPart)) / frequency.QuadPart;
    }

    float elapsedInMs() const {
        return 1e-3 * elapsed.QuadPart;
    }

    float elapsedInUs() const {
        return elapsed.QuadPart;
    }

private:
    LARGE_INTEGER begin, elapsed, frequency;
};
