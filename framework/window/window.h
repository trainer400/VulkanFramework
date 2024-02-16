#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>
#include <map>

namespace framework
{
    class Window
    {
    public:
        /**
         * @brief Construct a new Window object
         *
         * @param width Window's width (pixel)
         * @param height Window's height (pixel)
         * @param title Window's title
         * @param resizable Resizable flag, default = true
         */
        Window(int width, int height, const char *title, bool resizable = true);
        ~Window() { stop(); }

        /**
         * @brief Run the loop specifying the update function to call
         *
         * @param updateCallback update lambda function to call when updating the window
         */
        void run(std::function<void()> updateCallback, std::function<void()> updateSizeCallback, std::function<void()> closeCallback);

        // Setters
        void setSize(int width, int height);
        void addKeyCallback(int key, std::function<void(int key, int action)> callback);
        void removeKeyCallback(int key);
        void setPosCallback(std::function<void(double xpos, double ypos)> callback);
        void removePosCallback();

        // Getters
        inline int getWidth() { return width; }
        inline int getHeight() { return height; }
        inline int getKeyStatus(int key) { return glfwGetKey(window, key); }
        inline GLFWwindow *getWindow() { return window; }

    private:
        /**
         * @brief Stop function, called when glfwWindowShouldClose is true
         */
        void stop();

        /**
         * @brief Callback function called from the GLFW window. It looks inside the callbacks map if
         * a correspondent callback is registered and if so it calls it.
         */
        void keyEventCallback(int key, int scancode, int action, int mods);

        /**
         * @brief Callback function called from the GLFW window. It looks if a callback is registered
         * by the user and in case calls it.
         */
        void mousePosCallback(double xpos, double ypos);

        int width, height;
        GLFWwindow *window = nullptr;

        // Vector of key callbacks
        std::map<int, std::function<void(int key, int action)>> keyCallbacks;

        // Mouse pos callback
        std::function<void(double xpos, double ypos)> posCallback;
    };
}