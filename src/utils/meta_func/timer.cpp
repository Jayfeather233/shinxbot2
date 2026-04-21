#include "timer.h"

void Timer::run()
{
    while (running.load()) {
        std::this_thread::sleep_for(interval);
        if (running.load()) {
            std::map<std::string, std::vector<std::function<void(bot * p)>>>
                snapshot;
            {
                std::lock_guard<std::mutex> lock(callbacks_mutex);
                snapshot = callbacks;
            }

            for (auto u : snapshot) {
                for (auto f : u.second) {
                    try {
                        f(this->p);
                    }
                    catch (char *e) {
                        p->cq_send_all_op(
                            (std::string) "Timer Throw an char*: " + e);
                        p->setlog(LOG::ERROR,
                                  (std::string) "Timer Throw an char*: " + e);
                    }
                    catch (std::string e) {
                        p->cq_send_all_op("Timer Throw an string: " + e);
                        p->setlog(LOG::ERROR, "Timer Throw an string: " + e);
                    }
                    catch (std::exception &e) {
                        p->cq_send_all_op(
                            (std::string) "Timer Throw an exception: " +
                            e.what());
                        p->setlog(LOG::ERROR,
                                  (std::string) "Timer Throw an exception: " +
                                      e.what());
                    }
                    catch (...) {
                        p->cq_send_all_op("Timer Throw an unknown error");
                        p->setlog(LOG::ERROR, "Timer Throw an unknown error");
                    }
                }
            }
        }
    }
}

Timer::Timer(std::chrono::duration<double> dur, bot *p)
    : interval(dur), running(false), p(p)
{
}

void Timer::set_interval(std::chrono::duration<double> dur) { interval = dur; }
void Timer::add_callback(const std::string &name,
                         std::function<void(bot *p)> cb)
{
    std::lock_guard<std::mutex> lock(callbacks_mutex);
    this->callbacks[name].push_back(cb);
}
void Timer::remove_callback(const std::string &name)
{
    std::lock_guard<std::mutex> lock(callbacks_mutex);
    this->callbacks.erase(name);
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