#pragma once

#include <glm/glm.hpp>

namespace framework
{
    /**
     * @brief This class abstracts a camera in the space.
     * @note All the angles are expressed in degrees.
     * Positive pitch angles will result in camera looking up,
     * positive yaw angles will cause the camera looking to the right
     */
    class Camera
    {
    public:
        Camera(float fovY, float nearPlane, float farPlane);

        // Setters
        inline void setPosition(const glm::vec3 &position) { this->position = position; }
        inline void setFovY(float fov) { this->fovy = fov; }
        void setDirection(float yaw, float pitch);
        void lookAt(const glm::vec3 &point);

        // Getters
        glm::mat4 getLookAtMatrix();
        glm::mat4 getPerspectiveMatrix(uint32_t width, uint32_t height);
        glm::vec3 getPosition() { return position; }
        glm::vec3 getDirection() { return direction; }

    private:
        // Vector that indicates the upper axis of rotation for the camera
        glm::vec3 rotationAxis;

        // Position in the space
        glm::vec3 position{0, 0, 0};

        // Camera direction
        glm::vec3 direction{0, 0, 1};

        // Perspective parameters
        float fovy = 45, nearPlane = 0, farPlane = 0;
    };
}