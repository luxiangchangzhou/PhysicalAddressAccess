#include "LX_Timer.h"


using namespace std;

LX_Timer::LX_Timer() {
}


LX_Timer::~LX_Timer() {
}

void LX_Timer::start_ms(long long ms) {
    begin_time = chrono::high_resolution_clock::now();
    this->ms = ms;
}
void LX_Timer::start_micro(long long micro) {
    begin_time = chrono::high_resolution_clock::now();
    this->micro = micro;
}

long long LX_Timer::get_ms() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - begin_time).count() + ms;
}
long long LX_Timer::get_micro() {
    return chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - begin_time).count() + micro;
}
