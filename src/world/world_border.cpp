#include "world_border.h"

void WorldBorder::calculatePortalTeleportBoundary() {
    // Define a fixed padding, e.g., 1000 meters inside the border
    constexpr double padding = 0;
    portalTeleportBoundary = getHalfSize() - padding;

    // Ensure the boundary is positive
    if (portalTeleportBoundary < 0.0) {
        portalTeleportBoundary = 0.0;
    }
}

void WorldBorder::initialize(const WorldBorderConfig& config) {
    if (isInitialized) {
        return;
    }
    size = config.size;
    centerX = config.center[0];
    centerZ = config.center[1];
    warningTime = config.warningTime;
    warningBlocks = config.warningBlocks;
    biggestSize = config.size;
    calculatePortalTeleportBoundary();
    isInitialized = true;
}

void WorldBorder::updateSize(double newSize) {
    std::lock_guard lock(borderMutex);
    size = newSize;
    calculatePortalTeleportBoundary();
}

void WorldBorder::updateCenter(double newCenterX, double newCenterZ) {
    std::lock_guard lock(borderMutex);
    centerX = newCenterX;
    centerZ = newCenterZ;
}

void WorldBorder::updateWarningTime(int32_t newWarningTime) {
    std::lock_guard lock(borderMutex);
    warningTime = newWarningTime;
}

void WorldBorder::updateWarningDistance(int32_t newWarningBlocks) {
    std::lock_guard lock(borderMutex);
    warningBlocks = newWarningBlocks;
}

void WorldBorder::reInitialize() {
    std::lock_guard lock(borderMutex);
    biggestSize = size;
}
