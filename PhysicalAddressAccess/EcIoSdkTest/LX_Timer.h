#pragma once
#include <chrono>

//基于标准库的高精度定时器

class LX_Timer {
  public:
    LX_Timer();
    ~LX_Timer();
    void start_ms(long long ms);
    void start_micro(long long micro);

    long long get_ms();
    long long get_micro();

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_time;
    long long ms;
    long long micro;

};

