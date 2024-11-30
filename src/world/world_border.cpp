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
    size = config.size;
    centerX = config.center[0];
    centerZ = config.center[1];
    warningTime = config.warningTime;
    warningBlocks = config.warningBlocks;
    calculatePortalTeleportBoundary();
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