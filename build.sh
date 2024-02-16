# Compile the shaders
# glslc shaders/visualizer.vert -o shaders/vert.spv
# glslc shaders/visualizer.frag -o shaders/frag.spv

# Create the build directory where to put all the cmake stuff
mkdir build

# Create cmake files
cd build
cmake -G Ninja ../ && cmake --build .

# Move the result into the main directory
mv cube ../cube
