#ifndef WORLD_BORDER_H
#define WORLD_BORDER_H
#include "core/config.h"

struct WorldBorder {
    double size{};
    double centerX{};
    double centerZ{};
    int32_t warningTime{};    // Seconds
    int32_t warningBlocks{};  // Meters
    double portalTeleportBoundary{}; // Calculated automatically

    std::mutex borderMutex;  // To protect border updates


    WorldBorder() = default;

    void initialize(const WorldBorderConfig& config);

    double getHalfSize() const { return size / 2.0; }

    // Calculate portalTeleportBoundary based on world border size
    void calculatePortalTeleportBoundary();
    // Update the border size and recalculate portalTeleportBoundary
    void updateSize(double newSize);
    void updateCenter(double newCenterX, double newCenterZ);
};

#endif //WORLD_BORDER_H
