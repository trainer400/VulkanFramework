cmake_minimum_required(VERSION 3.16)
set(CMAKE_MINIMUM_REQUIRED_VERSION ON)
set(CMAKE_CXX_STANDARD 20)

set(FRAMEWORK_CORE 
    core/commandBuffer.cpp
    core/commandPool.cpp
    core/descriptorSet.cpp
    core/drawableCollection.cpp
    core/frameBufferCollection.cpp
    core/pipeline.cpp
    core/renderPass.cpp
    core/shader.cpp
    core/swapChain.cpp
    core/texture.cpp
    core/vulkan.cpp
    core/semaphore.cpp
    core/fence.cpp
)

set(FRAMEWORK_DEVICES
    devices/logicalDevice.cpp
    devices/physicalDevice.cpp
)

set(FRAMEWORK_UTILS
    utils/camera.cpp
    utils/constantVelocityCounter.cpp
    utils/objectParser.cpp
    utils/FPSCamera.cpp
    utils/defaultRenderer.cpp
)

set(FRAMEWORK_WINDOW
    window/window.cpp
    window/windowSurface.cpp
)

set(IMGUI
    libs/ImGui/imgui.cpp
    libs/ImGui/imgui_demo.cpp
    libs/ImGui/imgui_draw.cpp
    libs/ImGui/imgui_widgets.cpp
    libs/ImGui/imgui_tables.cpp
    libs/ImGui/backends/imgui_impl_glfw.cpp
    libs/ImGui/backends/imgui_impl_vulkan.cpp
)

set(IMPLOT
    libs/ImPlot/implot.cpp
    libs/ImPlot/implot_items.cpp
    libs/ImPlot/implot_demo.cpp    
)

add_library(framework ${FRAMEWORK_CORE} ${FRAMEWORK_DEVICES} ${FRAMEWORK_UTILS} ${FRAMEWORK_WINDOW} ${IMGUI} ${IMPLOT})
include_directories(framework ${CMAKE_CURRENT_SOURCE_DIR}/)
include_directories(framework ${CMAKE_CURRENT_SOURCE_DIR}/libs)
include_directories(framework ${CMAKE_CURRENT_SOURCE_DIR}/libs/ImGui)
include_directories(framework ${CMAKE_CURRENT_SOURCE_DIR}/libs/ImPlot)
