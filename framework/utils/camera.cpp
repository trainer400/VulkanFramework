#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>

namespace framework
{
    Camera::Camera(float fov_y, float near_plane, float far_plane)
    {
        if (near_plane <= 0 || far_plane <= 0 || near_plane == far_plane)
        {
            throw std::runtime_error("[Camera] Bad near/far plane");
        }

        this->near_plane = near_plane;
        this->far_plane = far_plane;
        this->fovy = fov_y;
    }

    void Camera::setDirection(float yaw, float pitch)
    {
        direction.x = sin(glm::radians(yaw)) * cos(glm::radians(-pitch));
        direction.y = sin(glm::radians(-pitch));
        direction.z = cos(glm::radians(yaw)) * cos(glm::radians(-pitch));

        direction = glm::normalize(direction);
    }

    void Camera::lookAt(const glm::vec3 &point)
    {
        // Compute the difference between the point to look at and the position
        glm::vec3 dir = point - position;

        direction = glm::normalize(dir);
    }

    glm::mat4 Camera::getLookAtMatrix()
    {
        return glm::lookAt(position, position + glm::normalize(direction), glm::vec3{0.0f, 1.0f, 0.0f});
    }

    glm::mat4 Camera::getPerspectiveMatrix(uint32_t width, uint32_t height)
    {
        glm::mat4 prj = glm::perspective(glm::radians(fovy), width / (float)height, near_plane, far_plane);
        // Conversion from GL to Vulkan, Y axis inverted
        prj[1][1] *= -1;
        return prj;
    }
}