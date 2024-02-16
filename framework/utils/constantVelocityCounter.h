#pragma once

#include <chrono>

namespace framework
{
    class ConstantVelocityCounter
    {
    public:
        /**
         * @brief Construct a new Constant Velocity Counter object
         *
         * @param velocity The velocity in unit/s
         */
        ConstantVelocityCounter(float velocity);

        /**
         * @brief Estimates the time elapsed since the last time and updates
         * the position value depending on the velocity
         */
        void updateCounting();

        // Counting management
        void startCounting();
        void stopCounting();

        // Getters and setters
        inline void setPosition(float position) { this->position = position; }
        inline void setVelocity(float velocity) { this->velocity = velocity; }
        inline float getPosition() { return position; }

    private:
        bool counting = false;
        std::chrono::_V2::system_clock::time_point lastUpdate;

        float velocity;
        float position = 0;
    };
}