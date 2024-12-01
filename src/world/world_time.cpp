#include "world_time.h"

#include <valarray>

void WorldTime::tick() {
    std::lock_guard lock(timeMutex);
    worldAge += 1;
    timeOfDay = (timeOfDay + 1) % 24000; // Loop around after a full day
}

int64_t WorldTime::getWorldAge() {
    std::lock_guard lock(timeMutex);
    return worldAge;
}

int64_t WorldTime::getTimeOfDay() {
    std::lock_guard lock(timeMutex);
    return timeOfDay;
}

int64_t WorldTime::getDays() {
    std::lock_guard lock(timeMutex);
    return worldAge / 24000;
}

void WorldTime::setTimeOfDay(int64_t newTime) {
    std::lock_guard lock(timeMutex);
    if (newTime < 0) {
        timeOfDay = std::abs(newTime);

    } else {
        timeOfDay = newTime % 24000;
    }
}
