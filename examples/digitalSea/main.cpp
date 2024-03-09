#include <iostream>
#include <libs/glm/glm.hpp>
#include <libs/glm/gtc/matrix_transform.hpp>
#include <framework/core/vulkan.h>
#include <framework/utils/defaultRenderer.h>
#include <framework/core/uniformBuffer.h>
#include <memory>

using namespace std;
using namespace framework;

shared_ptr<Vulkan> vulkan;
shared_ptr<Window> window;
shared_ptr<LogicalDevice> lDevice;
shared_ptr<CommandPool> commandPool;
unique_ptr<CommandBuffer> commandBuffer;
shared_ptr<Pipeline> objPipeline;
unique_ptr<SwapChain> swapChain;
unique_ptr<RenderPass> renderPass;
unique_ptr<FrameBufferCollection> frameBufferCollection;
unique_ptr<DefaultRenderer> renderer;

int main()
{
    cout << "Hello World!" << endl;
    return 0;
}