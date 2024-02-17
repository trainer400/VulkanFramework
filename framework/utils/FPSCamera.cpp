#include "FPSCamera.h"

#include <stdexcept>

#include <iostream>

using namespace framework;

namespace RocketFlightVisualizer
{
    FPSCamera::FPSCamera(float velocity, int fovY, float nearPlane, float farPlane, FPSCameraKeyBindings bindings)
        : camera(fovY, nearPlane, farPlane), bindings(bindings),
          forward(velocity), backward(velocity), left(velocity),
          right(velocity), up(velocity), down(velocity)
    {
    }

    FPSCamera::~FPSCamera()
    {
        if (registered && window != nullptr)
        {
            window->removeKeyCallback(bindings.forwardKey);
            window->removeKeyCallback(bindings.backwardKey);
            window->removeKeyCallback(bindings.leftKey);
            window->removeKeyCallback(bindings.rightKey);
            window->removeKeyCallback(bindings.upKey);
            window->removeKeyCallback(bindings.downKey);
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
                                    float deltaYaw = (xpos - this->window->getWidth() / 2) * 0.08;
                                    float deltaPitch = (ypos - this->window->getHeight() / 2) * 0.08;

                                    yaw += deltaYaw;
                                    pitch += deltaPitch;

                                    if(pitch > 89.f){pitch = 89.f;}
                                    if(pitch < -89.f) { pitch = -89.f;}

                                    if(yaw > 360) { yaw = 0;} 
                                    if(yaw < 0) { yaw = 360;} });

        window->addKeyCallback(bindings.forwardKey, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        forward.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        forward.stopCounting();
                                    } });

        window->addKeyCallback(bindings.backwardKey, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        backward.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        backward.stopCounting();
                                    } });

        window->addKeyCallback(bindings.leftKey, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        left.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        left.stopCounting();
                                    } });

        window->addKeyCallback(bindings.rightKey, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        right.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        right.stopCounting();
                                    } });

        window->addKeyCallback(bindings.downKey, [&, this](int key, int acting)
                               {
                                    if (acting == GLFW_PRESS)
                                    {
                                        down.startCounting();
                                    }
                                    else if (acting == GLFW_RELEASE)
                                    {
                                        down.stopCounting();
                                    } });

        window->addKeyCallback(bindings.upKey, [&, this](int key, int acting)
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