#include "first_app.hpp"

#include <stdexcept>
#include <array>

namespace pve
{

    static glm::vec3 red = {1.0f, 0.0f, 0.0f};
    static glm::vec3 green = {0.0f, 1.0f, 0.0f};
    static glm::vec3 blue = {0.0f, 0.0f, 1.0f};

    FirstApp::FirstApp()
    {
        loadModels();
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }

    FirstApp::~FirstApp()
    {
        vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run()
    {
        // while the window doesn't want to close, poll window events
        while (!pveWindow.shouldClose())
        {
            glfwPollEvents();
            drawFrame();
        }

        // this makes the CPU block until all GPU operations have completed
        vkDeviceWaitIdle(pveDevice.device());
    }

    void FirstApp::sierpinski(
        std::vector<PveModel::Vertex> &vertices,
        int depth,
        glm::vec2 left,
        glm::vec2 right,
        glm::vec2 top)
    {
        if (depth <= 0)
        {
            vertices.push_back({top, red});
            vertices.push_back({right, green});
            vertices.push_back({left, blue});
        }
        else
        {
            auto leftTop = 0.5f * (left + top);
            auto rightTop = 0.5f * (right + top);
            auto leftRight = 0.5f * (left + right);
            sierpinski(vertices, depth - 1, left, leftRight, leftTop);
            sierpinski(vertices, depth - 1, leftRight, right, rightTop);
            sierpinski(vertices, depth - 1, leftTop, rightTop, top);
        }
    }

    void FirstApp::loadModels()
    {
        // this will initialize vertex data positions
        // std::vector<PveModel::Vertex> vertices {
        //     {{0.0f,-0.5f},red},
        //     {{0.5f,0.5f},green},
        //     {{-0.5f,0.5f},blue}
        // };
        std::vector<PveModel::Vertex> vertices{};
        sierpinski(vertices, 5,
                   {-0.5f, 0.5f},
                   {0.5f, 0.5f},
                   {0.0f, -0.5f});
        pveModel = std::make_unique<PveModel>(pveDevice, vertices);
    }

    void FirstApp::createPipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout");
        }
    }

    void FirstApp::createPipeline()
    {
        auto pipelineConfig = PvePipeline::defaultPipelineConfigInfo(
            pveSwapChain->width(), pveSwapChain->height());
        // a render pass describes the structure and format of our frame buffer objects and their attachments
        // it's a blueprint that tells a graphics pipeline object what layout to expect from the output frame buffer
        // when it's time to actually render, the graphics pipeline is already prepared to output to our frame buffer,
        // as long as the passed frame buffer object is setup in a way that is compatible with what we specified in the render pass.
        // multiple subpasses can be grouped together into a single render pass
        pipelineConfig.renderPass = pveSwapChain->getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        pvePipeline = std::make_unique<PvePipeline>(
            pveDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }

    void FirstApp::recreateSwapChain(){
        // get the current window size
        auto extent = pveWindow.getExtent();
        // while the window has at least one sizeless dimension the program will pause and wait
        // this can occur during minimization, for example
        while (extent.width == 0 || extent.height == 0){
            extent = pveWindow.getExtent();
            glfwWaitEvents();
        }
        // wait until the current swap chain is no longer being used before we create the new swap chain
        vkDeviceWaitIdle(pveDevice.device());
        pveSwapChain = std::make_unique<PveSwapChain>(pveDevice, extent);
    }

    void FirstApp::createCommandBuffers()
    {
        commandBuffers.resize(pveSwapChain->imageCount());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = pveDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(pveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers");
        }
    }

    void FirstApp::recordCommandBuffer(int imageIndex)
    {

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = pveSwapChain->getRenderPass();
        renderPassInfo.framebuffer = pveSwapChain->getFrameBuffer(imageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = pveSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // commandBuffer in each render pass will bind our graphics pipeline,
        // then bind our model, which contains the vertex data, and then record a
        // command buffer to draw every vertex contained by the model
        pvePipeline->bind(commandBuffers[imageIndex]);
        pveModel->bind(commandBuffers[imageIndex]);
        pveModel->draw(commandBuffers[imageIndex]);

        vkCmdEndRenderPass(commandBuffers[imageIndex]);

        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

    void FirstApp::drawFrame()
    {

        uint32_t imageIndex;
        auto result = pveSwapChain->acquireNextImage(&imageIndex);

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image");
        }

        result = pveSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image");
        }
    }

} // namespace pve
