#include "world_time.h"

#include <valarray>

void WorldTime::tick() {
    std::lock_guard lock(timeMutex);
    worldAge += 20;
    timeOfDay = (timeOfDay + 20) % 24000; // Loop around after a full day
}

int64_t WorldTime::getWorldAge() {
    std::lock_guard lock(timeMutex);
    return worldAge;
}

int64_t WorldTime::getTimeOfDay() {
    std::lock_guard lock(timeMutex);
    return timeOfDay;
}

void WorldTime::setTimeOfDay(int64_t newTime) {
    std::lock_guard lock(timeMutex);
    if (newTime < 0) {
        timeOfDay = std::abs(newTime);

    } else {
        timeOfDay = newTime % 24000;
    }
}