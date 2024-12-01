#include "first_app.hpp"

#define GLM_FORCE_RADIANS  // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>

namespace pve {

// temporary
struct SimplePushConstantData {
    glm::mat2 transform{1.f};  // initialized as an identity matrix
    glm::vec2 offset;
    alignas(16) glm::vec3 color;  // alignas: tutorial 9 at 09:00
};

static glm::vec3 red = {1.0f, 0.0f, 0.0f};
static glm::vec3 green = {0.0f, 1.0f, 0.0f};
static glm::vec3 blue = {0.0f, 0.0f, 1.0f};

FirstApp::FirstApp() {
    loadGameObjects();
    createPipelineLayout();
    createPipeline();
}

FirstApp::~FirstApp() {
    vkDestroyPipelineLayout(pveDevice.device(), pipelineLayout, nullptr);
}

void FirstApp::run() {
    // while the window doesn't want to close, poll window events
    while (!pveWindow.shouldClose()) {
        glfwPollEvents();

        // the beginFrame function returns a nullptr if the swap chains needs to be recreated
        if (auto commandBuffer = pveRenderer.beginFrame()) {
            pveRenderer.beginSwapChainRenderPass(commandBuffer);
            renderGameObjects(commandBuffer);
            pveRenderer.endSwapChaniRenderPass(commandBuffer);
            pveRenderer.endFrame();
        }
    }

    // this makes the CPU block until all GPU operations have completed
    vkDeviceWaitIdle(pveDevice.device());
}

void FirstApp::sierpinski(std::vector<PveModel::Vertex> &vertices, int depth,
                          glm::vec2 left, glm::vec2 right, glm::vec2 top) {
    if (depth <= 0) {
        vertices.push_back({top, red});
        vertices.push_back({right, green});
        vertices.push_back({left, blue});
    } else {
        auto leftTop = 0.5f * (left + top);
        auto rightTop = 0.5f * (right + top);
        auto leftRight = 0.5f * (left + right);
        sierpinski(vertices, depth - 1, left, leftRight, leftTop);
        sierpinski(vertices, depth - 1, leftRight, right, rightTop);
        sierpinski(vertices, depth - 1, leftTop, rightTop, top);
    }
}

void FirstApp::sierpinskiTriangle() {
    std::vector<PveModel::Vertex> vertices{};
    sierpinski(vertices, 5, {-0.5f, 0.5f}, {0.5f, 0.5f}, {0.0f, -0.5f});

    auto pveModel = std::make_shared<PveModel>(pveDevice, vertices);

    auto triangle = PveGameObject::createGameObject();
    triangle.model = pveModel;
    triangle.color = {.1f, .8f, .1f};
    gameObjects.push_back(std::move(triangle));
}

void FirstApp::spinningTriangle() {
    // this will initialize vertex data positions
    std::vector<PveModel::Vertex> vertices{
        {{0.0f, -0.5f}, red}, {{0.5f, 0.5f}, green}, {{-0.5f, 0.5f}, blue}};

    // make_shared allows us to use one model instance that can be used by multiple game objects
    // and it will stay in memory as long as one game object is still using it
    auto pveModel = std::make_shared<PveModel>(pveDevice, vertices);

    auto triangle = PveGameObject::createGameObject();
    triangle.model = pveModel;
    triangle.color = {.1f, .8f, .1f};
    triangle.transform2d.translation.x = .2f;  // move slightly to the right
    // this scales the x axis to double its size and the y axis to half its size,
    // resulting in a wide and short triangle.
    triangle.transform2d.scale = {2.f, .5f};
    triangle.transform2d.rotation = .25f * glm::two_pi<float>();
    gameObjects.push_back(std::move(triangle));
}

void FirstApp::crazyTriangles() {
    std::vector<PveModel::Vertex> vertices{{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                           {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                           {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
    auto pveModel = std::make_shared<PveModel>(pveDevice, vertices);

    // https://www.color-hex.com/color-palette/5361
    std::vector<glm::vec3> colors{{1.f, .7f, .73f},
                                  {1.f, .87f, .73f},
                                  {1.f, 1.f, .73f},
                                  {.73f, 1.f, .8f},
                                  {.73, .88f, 1.f}};
    for (auto &color : colors) {
        color = glm::pow(color, glm::vec3{2.2f});
    }
    for (int i = 0; i < 40; i++) {
        auto triangle = PveGameObject::createGameObject();
        triangle.model = pveModel;
        triangle.transform2d.scale = glm::vec2(.5f) + i * 0.025f;
        triangle.transform2d.rotation = i * glm::pi<float>() * .025f;
        triangle.color = colors[i % colors.size()];
        gameObjects.push_back(std::move(triangle));
    }
}

void FirstApp::loadGameObjects() { crazyTriangles(); }

void FirstApp::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pveDevice.device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void FirstApp::createPipeline() {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    // a render pass describes the structure and format of our frame buffer objects and their attachments
    // it's a blueprint that tells a graphics pipeline object what layout to expect from the output frame buffer
    // when it's time to actually render, the graphics pipeline is already prepared to output to our frame buffer,
    // as long as the passed frame buffer object is setup in a way that is compatible with what we specified in the render pass.
    // multiple subpasses can be grouped together into a single render pass
    pipelineConfig.renderPass = pveRenderer.getSwapChainRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    pvePipeline =
        std::make_unique<PvePipeline>(pveDevice, "shaders/simple_shader.vert.spv",
                                      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) {
    // update
    int i = 0;
    for (auto &obj : gameObjects) {
        i += 1;
        obj.transform2d.rotation = glm::mod<float>(obj.transform2d.rotation + 0.001f * i,
                                                   2.f * glm::pi<float>());
    }
    // render
    pvePipeline->bind(commandBuffer);
    for (auto &obj : gameObjects) {
        SimplePushConstantData push{};
        push.offset = obj.transform2d.translation;
        push.color = obj.color;
        push.transform = obj.transform2d.mat2();
        vkCmdPushConstants(commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(SimplePushConstantData), &push);
        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

}  // namespace pve
