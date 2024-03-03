cmake_minimum_required(VERSION 3.16)
set(CMAKE_MINIMUM_REQUIRED_VERSION ON)
set(CMAKE_CXX_STANDARD 20)

project("Examples")
add_compile_options(-O3 -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi)
add_compile_definitions(GLM_FORCE_LEFT_HANDED GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)

add_subdirectory(framework/)

# Rotating cube
add_executable(cube examples/cube/main.cpp)
target_link_libraries(cube PUBLIC framework vulkan glfw)
target_include_directories(cube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories(cube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework)
target_include_directories(cube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework/libs)
target_include_directories(cube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework/libs/ImGui)
target_include_directories(cube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework/libs/ImPlot)

# Obj file loader
add_executable(OBJeffect examples/OBJeffect/main.cpp)
target_link_libraries(OBJeffect PUBLIC framework vulkan glfw)
target_include_directories(OBJeffect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories(OBJeffect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework)
target_include_directories(OBJeffect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework/libs)
target_include_directories(OBJeffect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework/libs/ImGui)
target_include_directories(OBJeffect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework/libs/ImPlot)