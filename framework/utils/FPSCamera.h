#pragma once

#include <utils/camera.h>
#include <utils/constantVelocityCounter.h>
#include <window/window.h>

#include <memory>

namespace framework
{
    struct FPSCameraKeyBindings
    {
        uint32_t forwardKey = GLFW_KEY_W;
        uint32_t backwardKey = GLFW_KEY_S;
        uint32_t leftKey = GLFW_KEY_A;
        uint32_t rightKey = GLFW_KEY_D;
        uint32_t upKey = GLFW_KEY_SPACE;
        uint32_t downKey = GLFW_KEY_LEFT_SHIFT;
    };

    class FPSCamera
    {
    public:
        FPSCamera(float velocity, int fovY, float nearPlane, float farPlane, FPSCameraKeyBindings bindings);
        ~FPSCamera();

        /**
         * @brief Registers the callbacks for all the specified bindings
         */
        void registerCallbacks(std::shared_ptr<framework::Window> window);

        /**
         * @brief Method to update the velocity counters and register the movement
         */
        void updatePosition();

        // Setters
        inline void setPosition(glm::vec3 position) { camera.setPosition(position); }
        inline void setFovY(float fov) { camera.setFovY(fov); }
        inline void setVelocity(float v)
        {
            forward.setVelocity(v);
            backward.setVelocity(v);
            left.setVelocity(v);
            right.setVelocity(v);
            up.setVelocity(v);
            down.setVelocity(v);
        }

        // Getters
        inline glm::mat4 getLookAtMatrix() { return camera.getLookAtMatrix(); }
        inline glm::mat4 getPerspectiveMatrix(uint32_t width, uint32_t height) { return camera.getPerspectiveMatrix(width, height); }
        inline glm::vec3 getPosition() { return camera.getPosition(); }
        inline glm::vec3 getDirection() { return camera.getDirection(); }

    private:
        FPSCameraKeyBindings bindings;

        // Use the framework camera to compute the matrices
        framework::Camera camera;
        std::shared_ptr<framework::Window> window;

        // Registered callbacks
        bool registered = false;

        // Camera direction
        float yaw = 0, pitch = 0;

        // Constant velocity counters to make the speed time dependent
        framework::ConstantVelocityCounter forward;
        framework::ConstantVelocityCounter backward;
        framework::ConstantVelocityCounter left;
        framework::ConstantVelocityCounter right;
        framework::ConstantVelocityCounter up;
        framework::ConstantVelocityCounter down;
    };
}