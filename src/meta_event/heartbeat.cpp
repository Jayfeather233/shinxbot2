#include "heartbeat.h"

static inline std::time_t get_current_time()
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

static void wait_child(pid_t id) { waitpid(id, NULL, 0); }

void heartBeat::start_recover()
{
    pid_t k = fork();
    if (k == -1) {
        std::cerr << "Process Error!" << std::endl;
    }
    else if (k == 0) {
        for (const std::string &u : recover_commands) {
            int rc = system(u.c_str());
            if (rc == -1) {
                std::cerr << "recover command failed to launch: " << u
                          << std::endl;
            }
        }
        exit(0);
    }
    else {
        std::thread(wait_child, k).detach();
    }
}
heartBeat::heartBeat(const std::vector<std::string> &commands)
    : recover_commands(commands), las_time(get_current_time())
{
    is_recorded = false;
    recovering = true;
}
void heartBeat::run()
{
    while (running_.load()) {
        if (is_recorded && get_current_time() - las_time >= 3600 * 8 &&
            !recovering) { // 8h
            start_recover();
            recovering = true;
        }
        sleep(60);
    }
}

void heartBeat::stop() { running_.store(false); }

void heartBeat::inform()
{
    las_time = get_current_time();
    is_recorded = true;
    recovering = false;
}