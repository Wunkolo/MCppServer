//
// Created by noah1 on 03.12.2024.
//

#include "weather.h"

#include "networking/clientbound_packets.h"

void Weather::setWeather(WeatherType type, int durationTicks) {
    currentWeather = type;
    switch (type) {
        case WeatherType::CLEAR:
            if (durationTicks > 0) {
                clearCounter = durationTicks;
            } else {
                setRandomClearDuration();
            }
            if (currentRainState) {
                setRainState(false);
            }
            if (currentThunderState) {
                setThunderState(false);
            }
            break;
        case WeatherType::RAIN:
            clearCounter = 0;
            if (durationTicks > 0) {
                rainCounter = durationTicks;
            } else {
                setRandomRainDuration();
            }
            if (!currentRainState) {
                setRainState(true);
            }
            break;
        case WeatherType::THUNDER:
            clearCounter = 0;
            if (durationTicks > 0) {
                thunderCounter = durationTicks;
                rainCounter = durationTicks;
            } else {
                setRandomThunderAndRainDuration();
            }
            if (!currentRainState) {
                setRainState(true);
            }
            if (!currentThunderState) {
                setThunderState(true);
            }
            break;
    }
}

void Weather::handleTick() {
    std::lock_guard lock(mutex);

    // Handle clear counter
    if (clearCounter > 0) {
        clearCounter--;
        if (clearCounter == 0) {
            // Clear period ended, resume natural weather
            currentWeather = naturalWeatherState();
            notifyWeatherChange(currentWeather);
        }
        // Handle smooth transitions (lerping)
        handleLerping();

        // When clearCounter is active, override other weather states
        return;
    }

    // Handle rain counter
    if (rainCounter > 0) {
        rainCounter--;
        if (rainCounter == 0) {
            toggleRain();
        }
    }

    // Handle thunder counter
    if (thunderCounter > 0) {
        thunderCounter--;
        if (thunderCounter == 0) {
            toggleThunder();
        }
    }

    // Determine current weather based on rain and thunder states
    WeatherType newWeather = WeatherType::CLEAR;
    if (currentRainState) {
        newWeather = WeatherType::RAIN;
        if (currentThunderState) {
            newWeather = WeatherType::THUNDER;
        }
    }

    if (newWeather != currentWeather) {
        currentWeather = newWeather;
        notifyWeatherChange(currentWeather);
    }

    // Handle smooth transitions (lerping)
    handleLerping();
}

void Weather::notifyWeatherChange(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR:
            sendGameEvent(GameEvent::BeginRaining, 0.f);
        break;
        case WeatherType::RAIN:
            sendGameEvent(GameEvent::BeginRaining, 1.f); // BeginRaining triggered
        break;
        case WeatherType::THUNDER:
            sendGameEvent(GameEvent::BeginRaining, 1.f); // BeginRaining triggered
        break;
    }
}

// Set rain state with lerp
    void Weather::setRainState(bool enabled) {
        if (enabled != currentRainState) {
            currentRainState = enabled;
            targetRainLevel = enabled ? 1.0f : 0.0f;
            rainLerpTicksRemaining = TRANSITION_TICKS;
            // Calculate fixed delta once
            rainDelta = (targetRainLevel - currentRainLevel) / static_cast<float>(TRANSITION_TICKS);
            rainLerpTicksRemaining = TRANSITION_TICKS;
        }
    }

    // Set thunder state with lerp
    void Weather::setThunderState(bool enabled) {
        if (enabled != currentThunderState) {
            currentThunderState = enabled;
            targetThunderLevel = enabled ? 1.0f : 0.0f;
            thunderLerpTicksRemaining = TRANSITION_TICKS;
            // Calculate fixed delta once
            thunderDelta = (targetThunderLevel - currentThunderLevel) / static_cast<float>(TRANSITION_TICKS);
            thunderLerpTicksRemaining = TRANSITION_TICKS;
        }
    }

    // Set rain level instantly (used for clear)
    void Weather::setRainLevel(float level) {
        currentRainLevel = level;
        targetRainLevel = level;
        rainLerpTicksRemaining = 0;
        sendGameEvent(GameEvent::RainLevelChange, level);
    }

    // Set thunder level instantly (used for clear)
    void Weather::setThunderLevel(float level) {
        currentThunderLevel = level;
        targetThunderLevel = level;
        thunderLerpTicksRemaining = 0;
        sendGameEvent(GameEvent::ThunderLevelChange, level);
    }

    // Handle smooth transitions
    void Weather::handleLerping() {
        // Handle rain lerp
        if (rainLerpTicksRemaining > 0) {
            currentRainLevel += rainDelta;
            rainLerpTicksRemaining--;

            // Clamp values and ensure exact target is reached
            if (rainLerpTicksRemaining <= 0) {
                currentRainLevel = targetRainLevel;
            }

            sendGameEvent(GameEvent::RainLevelChange, currentRainLevel);
        }

        // Handle thunder lerp
        if (thunderLerpTicksRemaining > 0) {
            currentThunderLevel += thunderDelta;
            thunderLerpTicksRemaining--;

            // Clamp values and ensure exact target is reached
            if (thunderLerpTicksRemaining <= 0) {
                currentThunderLevel = targetThunderLevel;
            }

            sendGameEvent(GameEvent::ThunderLevelChange, currentThunderLevel);
        }
    }

    // Toggle rain on/off
    void Weather::toggleRain() {
        setRainState(!currentRainState);
        if (currentRainState) {
            // Rain started, reset counter for next toggle
            rainCounter = rng.getInt(RAIN_MIN_DURATION, RAIN_MAX_DURATION);
        } else {
            // Rain stopped, reset counter
            rainCounter = rng.getInt(CLEAR_MIN_DURATION, CLEAR_MAX_DURATION);
        }
    }

    // Toggle thunder on/off
    void Weather::toggleThunder() {
        setThunderState(!currentThunderState);
        if (currentThunderState) {
            // Thunder started, reset counter for next toggle
            thunderCounter = rng.getInt(THUNDER_MIN_DURATION, THUNDER_MAX_DURATION);
        } else {
            // Thunder stopped, reset counter
            thunderCounter = rng.getInt(CLEAR_MIN_DURATION, CLEAR_MAX_DURATION);
        }
    }

    // Determine natural weather state based on rain and thunder
    WeatherType Weather::naturalWeatherState() {
        if (currentRainState) {
            if (currentThunderState) {
                return WeatherType::THUNDER;
            }
            return WeatherType::RAIN;
        }
        return WeatherType::CLEAR;
    }
