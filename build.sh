# Compile the shaders
glslc examples/cube/shaders/cube.vert -o examples/cube/shaders/vert.spv
glslc examples/cube/shaders/cube.frag -o examples/cube/shaders/frag.spv

# Create the build directory where to put all the cmake stuff
mkdir build

# Create cmake files
cd build
cmake -G Ninja ../ && cmake --build .

# Move the result into the main directory
mv cube ../cube
mv OBJeffect ../OBJeffect
