#include "FPSCamera.h"
#include <stdexcept>

namespace framework
{
    FPSCamera::FPSCamera(float velocity, int fov_y, float near_plane, float far_plane, FPSCameraKeyBindings bindings)
        : camera(fov_y, near_plane, far_plane), bindings(bindings),
          forward(velocity), backward(velocity), left(velocity),
          right(velocity), up(velocity), down(velocity)
    {
    }

    FPSCamera::~FPSCamera()
    {
        if (registered && window != nullptr)
        {
            window->removeKeyCallback(bindings.forward_key);
            window->removeKeyCallback(bindings.backward_key);
            window->removeKeyCallback(bindings.left_key);
            window->removeKeyCallback(bindings.right_key);
            window->removeKeyCallback(bindings.up_key);
            window->removeKeyCallback(bindings.down_key);
            window->removePosCallback();

            registered = false;
        }
    }

    void FPSCamera::registerCallbacks(std::shared_ptr<Window> window)
    {
        if (window == nullptr)
        {
            throw std::runtime_error("[FPSCamera] Null window instance");
        }

        this->window = window;
        registered = true;

        // Window callbacks set
        window->setPosCallback([&, this](double xpos, double ypos)
                               {
                                    float delta_yaw = (xpos - this->window->getWidth() / 2) * 0.08;
                                    float delta_pitch = (ypos - this->window->getHeight() / 2) * 0.08;

                                    yaw += delta_yaw;
                                    pitch += delta_pitch;

                                    if(pitch > 89.f){pitch = 89.f;}
                                    if(pitch < -89.f) { pitch = -89.f;}

                                    if(yaw > 360) { yaw = 0;} 
                                    if(yaw < 0) { yaw = 360;} });

        window->addKeyCallback(bindings.forward_key, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        forward.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        forward.stopCounting();
                                    } });

        window->addKeyCallback(bindings.backward_key, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        backward.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        backward.stopCounting();
                                    } });

        window->addKeyCallback(bindings.left_key, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        left.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        left.stopCounting();
                                    } });

        window->addKeyCallback(bindings.right_key, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        right.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        right.stopCounting();
                                    } });

        window->addKeyCallback(bindings.down_key, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        down.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        down.stopCounting();
                                    } });

        window->addKeyCallback(bindings.up_key, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        up.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        up.stopCounting();
                                    } });
    }

    void FPSCamera::updatePosition()
    {
        // Update the camera position
        forward.updateCounting();
        backward.updateCounting();
        left.updateCounting();
        right.updateCounting();
        up.updateCounting();
        down.updateCounting();

        glm::vec3 position = camera.getPosition();

        position += (forward.getPosition() - backward.getPosition()) * camera.getDirection();
        position += (left.getPosition() - right.getPosition()) * glm::normalize(glm::cross(camera.getDirection(), {0, 1, 0}));
        position.y += up.getPosition() - down.getPosition();

        camera.setPosition(position);
        camera.setDirection(yaw, pitch);

        // Reset the counters
        forward.setPosition(0);
        backward.setPosition(0);
        left.setPosition(0);
        right.setPosition(0);
        up.setPosition(0);
        down.setPosition(0);
    }

}