#include "window.h"
#include <stdexcept>

namespace framework
{
    Window::Window(int width, int height, const char *title, bool resizable) : width(width), height(height)
    {
        // Check the parameters
        if (width < 0 || height < 0)
        {
            throw std::runtime_error("[Window] Invalid width or height (< 0)");
        }

        if (title == nullptr)
        {
            throw std::runtime_error("[Window] Nullptr title");
        }

        // Init the GLFW environment
        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("[Window] Error initializing the GLFW environment");
        }

        // Set the window to NOT use OpenGL API
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);

        // Instantiate the window with the previous set hints
        this->window = glfwCreateWindow(this->width, this->height, title, nullptr, nullptr);

        // Check window validity
        if (this->window == NULL)
        {
            throw std::runtime_error("[Window] Null window pointer");
        }

        // Set the pointer to this object to call the object's callback
        glfwSetWindowUserPointer(window, this);

        // Create the lambda to call the correct callback
        auto keyCallback = [](GLFWwindow *window, int key, int scancode, int action, int mods)
        {
            static_cast<Window *>(glfwGetWindowUserPointer(window))->keyEventCallback(key, scancode, action, mods);
        };

        // Set the key callback
        glfwSetKeyCallback(window, keyCallback);

        // Create the lambda to call the correct callback
        auto posCallback = [](GLFWwindow *window, double xpos, double ypos)
        {
            static_cast<Window *>(glfwGetWindowUserPointer(window))->mousePosCallback(xpos, ypos);
        };

        glfwSetCursorPosCallback(window, posCallback);
    }

    void Window::run(std::function<void()> updateCallback, std::function<void()> updateSizeCallback, std::function<void()> closeCallback)
    {
        while (!glfwWindowShouldClose(this->window))
        {
            // Update the events
            glfwPollEvents();

            int previousWidth = width;
            int previousHeight = height;

            // Get new size if resizable
            glfwGetWindowSize(window, &width, &height);

            // Check if the size has changed
            if (width != previousWidth || height != previousHeight)
            {
                updateSizeCallback();
            }

            // Call the user defined update callback
            updateCallback();
        }

        // Once the window should close, destroy the environment
        stop();

        // Call the user defined closing callback
        closeCallback();
    }

    void Window::setSize(int width, int height)
    {
        // Check the parameters
        if (width < 0 || height < 0)
        {
            throw std::runtime_error("[Window] Invalid width or height (< 0)");
        }

        glfwSetWindowSize(this->window, width, height);
    }

    void Window::addKeyCallback(int key, std::function<void(int key, int action)> callback)
    {
        // Check that the key is not already present
        if (keyCallbacks.contains(key))
        {
            return;
        }

        // Insert the pair
        keyCallbacks.insert(std::pair{key, callback});
    }

    void Window::removeKeyCallback(int key)
    {
        if (!keyCallbacks.contains(key))
        {
            return;
        }

        // Remove the key
        keyCallbacks.erase(key);
    }

    void Window::setPosCallback(std::function<void(double xpos, double ypos)> callback)
    {
        this->posCallback = callback;
    }

    void Window::removePosCallback()
    {
        this->posCallback = [](double xpos, double ypos) {};
    }

    void Window::stop()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Window::keyEventCallback(int key, int scancode, int action, int mods)
    {
        // Call the registered function if present
        if (keyCallbacks.contains(key))
        {
            keyCallbacks.at(key)(key, action);
        }
    }

    void Window::mousePosCallback(double xpos, double ypos)
    {
        // Check if the function is valid
        if (posCallback)
        {
            posCallback(xpos, ypos);
        }
    }
}