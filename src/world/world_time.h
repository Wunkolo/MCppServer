#ifndef WORLD_TIME_H
#define WORLD_TIME_H
#include <cstdint>
#include <mutex>

struct WorldTime {
    int64_t worldAge;    // Total ticks since the world was created
    int64_t timeOfDay;   // Current time of day in ticks

    std::mutex timeMutex; // Mutex to protect access to time variables

    WorldTime() : worldAge(0), timeOfDay(0) {}

    void tick();
    int64_t getWorldAge();
    int64_t getTimeOfDay();
    void setTimeOfDay(int64_t newTime);
};

#endif //WORLD_TIME_H
