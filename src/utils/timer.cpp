#include "timer.h"

void Timer::run()
{
    while (running.load()) {
        std::this_thread::sleep_for(interval);
        if (running.load()) {
            for (auto f : this->callbacks) {
                f(this->p);
            }
        }
    }
}

Timer::Timer(std::chrono::duration<double> dur, bot *p)
    : interval(dur), running(false), p(p)
{
}

void Timer::set_interval(std::chrono::duration<double> dur) { interval = dur; }
void Timer::add_callback(std::function<void(bot *p)> cb)
{
    this->callbacks.push_back(cb);
}

void Timer::timer_start()
{
    running.store(true);
    timerThread = std::thread(&Timer::run, this);
}

void Timer::timer_stop()
{
    running.store(false);
    if (timerThread.joinable()) {
        timerThread.join();
    }
}

Timer::~Timer() { timer_stop(); }