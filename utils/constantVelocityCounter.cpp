#include "constantVelocityCounter.h"

#include <chrono>

namespace framework
{
    ConstantVelocityCounter::ConstantVelocityCounter(float velocity) : velocity(velocity) {}

    void ConstantVelocityCounter::updateCounting()
    {
        if (counting)
        {
            // Take the time
            auto time = std::chrono::system_clock::now();

            // Compute the delta
            int64_t delta = std::chrono::duration_cast<std::chrono::milliseconds>(time - lastUpdate).count();

            // Progress the counting
            position += velocity * delta / 1000.f;

            // Update the last time
            lastUpdate = time;
        }
    }

    void ConstantVelocityCounter::startCounting()
    {
        if (!counting)
        {
            counting = true;

            // Set the last update time
            lastUpdate = std::chrono::system_clock::now();
        }
    }

    void ConstantVelocityCounter::stopCounting()
    {
        counting = false;
    }
}