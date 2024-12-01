#ifndef WORLD_BORDER_H
#define WORLD_BORDER_H
#include "core/config.h"

struct WorldBorder {
    bool isInitialized{};
    double size{};
    double centerX{};
    double centerZ{};
    int32_t warningTime{};    // Seconds
    int32_t warningBlocks{};  // Meters
    double portalTeleportBoundary{}; // Calculated automatically
    double biggestSize{}; // The biggest size the border has ever been

    std::mutex borderMutex;  // To protect border updates


    WorldBorder() = default;

    void initialize(const WorldBorderConfig& config);

    double getHalfSize() const { return size / 2.0; }

    // Calculate portalTeleportBoundary based on world border size
    void calculatePortalTeleportBoundary();
    // Update the border size and recalculate portalTeleportBoundary
    void updateSize(double newSize);
    void updateCenter(double newCenterX, double newCenterZ);
    void updateWarningTime(int32_t newWarningTime);
    void updateWarningDistance(int32_t newWarningBlocks);
    double getDiameter() const { return size; }
    void reInitialize();
};

#endif //WORLD_BORDER_H
