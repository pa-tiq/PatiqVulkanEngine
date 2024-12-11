#include "first_app.hpp"

#include "constants/colors.hpp"
#include "controllers/keyboard_movement_controller.hpp"
#include "pve/pve_buffer.hpp"
#include "pve/pve_camera.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <vector>

namespace pve {

float MAX_FRAME_TIME = 1.0f;

FirstApp::FirstApp() {
    globalPool = PveDescriptorPool::Builder(pveDevice)
                     .setMaxSets(PveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, PveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();
    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run() {
    std::vector<std::unique_ptr<PveBuffer>> uboBuffers(PveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<PveBuffer>(
            pveDevice,
            sizeof(GlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = PveDescriptorSetLayout::Builder(pveDevice)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                               .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(PveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        PveDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    SimpleRenderSystem simpleRenderSystem{pveDevice,
                                          pveRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};

    PointLightSystem pointLightSystem{pveDevice,
                                      pveRenderer.getSwapChainRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};
    PveCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));  // camera looks to the center of the cube
    // while the window doesn't want to close, poll window events

    // viewerObject has no model and won't be rendered. It's used to store the camera's current state.
    auto viewerObject = PveGameObject::createGameObject();
    viewerObject.transform.translation.z = -2.5f;
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!pveWindow.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;
        frameTime = glm::min(frameTime, MAX_FRAME_TIME);

        cameraController.moveInPlaneXZ(pveWindow.getGLFWWindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = pveRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        // the beginFrame function returns a nullptr if the swap chains needs to be recreated
        if (auto commandBuffer = pveRenderer.beginFrame()) {
            int frameIndex = pveRenderer.getFrameIndex();
            FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSets[frameIndex],
                gameObjects};

            // prepare and update objects in memory
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            pointLightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render - record draw calls
            pveRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            pveRenderer.endSwapChainRenderPass(commandBuffer);
            pveRenderer.endFrame();
        }
    }

    // this makes the CPU block until all GPU operations have completed
    vkDeviceWaitIdle(pveDevice.device());
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<PveModel> pveModel = PveModel::createModelFromFile(pveDevice, "models/cube.obj");
    auto cube = PveGameObject::createGameObject();
    cube.model = pveModel;
    cube.name = "cube";
    cube.transform.translation = {-2.0f, -0.2f, 0.f};
    cube.transform.scale = {.3f, .3f, .3f};
    gameObjects.emplace(cube.getId(), std::move(cube));

    pveModel = PveModel::createModelFromFile(pveDevice, "models/flat_vase.obj");
    auto flatVase = PveGameObject::createGameObject();
    flatVase.model = pveModel;
    flatVase.name = "flatVase";
    flatVase.transform.translation = {1.0f, .5f, 0.f};
    flatVase.transform.scale = {3.f, 3.f, 3.f};
    gameObjects.emplace(flatVase.getId(), std::move(flatVase));

    pveModel = PveModel::createModelFromFile(pveDevice, "models/smooth_vase.obj");
    auto smoothVase = PveGameObject::createGameObject();
    smoothVase.model = pveModel;
    smoothVase.name = "smoothVase";
    smoothVase.transform.translation = {2.0f, .5f, 0.f};
    smoothVase.transform.scale = {3.f, 3.f, 3.f};
    gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

    pveModel = PveModel::createModelFromFile(pveDevice, "models/quad.obj");
    auto floor = PveGameObject::createGameObject();
    floor.model = pveModel;
    floor.transform.translation = {0.f, .5f, 0.f};
    floor.transform.scale = {3.f, 1.f, 3.f};
    gameObjects.emplace(floor.getId(), std::move(floor));

    // auto pointLight = PveGameObject::makePointLight(0.2f);
    // gameObjects.emplace(pointLight.getId(), std::move(pointLight));

    std::vector<glm::vec3> lightColors{
        {1.f, .1f, .1f},
        {.1f, .1f, 1.f},
        {.1f, 1.f, .1f},
        {1.f, 1.f, .1f},
        {.1f, 1.f, 1.f},
        {1.f, 1.f, 1.f}  //
    };

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = PveGameObject::makePointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight = glm::rotate(
            glm::mat4(1.f),
            (i * glm::two_pi<float>()) / lightColors.size(),
            glm::vec3(0.f, -1.f, 0.f));
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
}

}  // namespace pve
