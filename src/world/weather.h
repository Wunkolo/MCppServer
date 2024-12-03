#ifndef WEATHER_H
#define WEATHER_H
#include <chrono>
#include <random>
#include <thread>

// Enum for different weather types
enum class WeatherType {
    CLEAR,
    RAIN,
    THUNDER
};

// Constants for duration ranges (in ticks)
constexpr int CLEAR_MIN_DURATION = 12000;
constexpr int CLEAR_MAX_DURATION = 180000;

constexpr int RAIN_MIN_DURATION = 12000;
constexpr int RAIN_MAX_DURATION = 24000;

constexpr int THUNDER_MIN_DURATION = 3600;
constexpr int THUNDER_MAX_DURATION = 15600;

// Duration for smooth transition (in ticks)
constexpr int TRANSITION_TICKS = 100; // 5 seconds at 20 ticks per second

// Utility to generate random numbers within a range
class RandomGenerator {
public:
    RandomGenerator()
        : mt(rd()) {}

    int getInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(mt);
    }

private:
    std::random_device rd;
    std::mt19937 mt;
};


class Weather {
public:
    Weather() :
        currentWeather(WeatherType::CLEAR),
        clearCounter(0),
        rainCounter(0),
        thunderCounter(0),
        stopFlag(false),
        currentRainLevel(0.0f),
        targetRainLevel(0.0f),
        rainLerpTicksRemaining(0),
        rainDelta(0.0f),
        currentThunderLevel(0.0f),
        targetThunderLevel(0.0f),
        thunderLerpTicksRemaining(0),
        thunderDelta(0.0f)
    {
        // Initialize with default durations
        setRandomRainDuration();
        setRandomThunderDuration();

        // Initial weather state is clear, so no rain or thunder
        currentRainState = false;
        currentThunderState = false;
    }

    // Set weather with specified type and duration
    void setWeather(WeatherType type, int durationTicks);

    void handleTick();

private:
    WeatherType currentWeather;

    // Counters for natural weather
    int clearCounter;    // Only active when /weather clear is used
    int rainCounter;
    int thunderCounter;

    bool currentRainState;
    bool currentThunderState;

    // Lerping variables
    float currentRainLevel;
    float targetRainLevel;
    int rainLerpTicksRemaining;
    float rainDelta;

    float currentThunderLevel;
    float targetThunderLevel;
    int thunderLerpTicksRemaining;
    float thunderDelta;

    RandomGenerator rng;
    std::mutex mutex;
    bool stopFlag;

    // Notify clients about weather change with smooth transition
    static void notifyWeatherChange(WeatherType type);

    // Set random durations based on weather type
    void setRandomClearDuration() {
        clearCounter = rng.getInt(CLEAR_MIN_DURATION, CLEAR_MAX_DURATION);
    }

    void setRandomRainDuration() {
        rainCounter = rng.getInt(RAIN_MIN_DURATION, RAIN_MAX_DURATION);
    }

    void setRandomThunderDuration() {
        thunderCounter = rng.getInt(THUNDER_MIN_DURATION, THUNDER_MAX_DURATION);
    }

    void setRandomThunderAndRainDuration() {
        int duration = rng.getInt(THUNDER_MIN_DURATION, THUNDER_MAX_DURATION);
        rainCounter = duration;
        thunderCounter = duration;
    }

    // Set rain state with lerp
    void setRainState(bool enabled);
    // Set thunder state with lerp
    void setThunderState(bool enabled);
    // Set rain level instantly (used for clear)
    void setRainLevel(float level);
    // Set thunder level instantly (used for clear)
    void setThunderLevel(float level);
    // Handle smooth transitions
    void handleLerping();
    // Toggle rain on/off
    void toggleRain();
    // Toggle thunder on/off
    void toggleThunder();
    // Determine natural weather state based on rain and thunder
    WeatherType naturalWeatherState();
};

#endif //WEATHER_H
