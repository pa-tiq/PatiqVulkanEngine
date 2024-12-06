#include "first_app.hpp"

#include "colors.hpp"
#include "keyboard_movement_controller.hpp"
#include "pve_camera.hpp"
#include "simple_render_system.hpp"

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

FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{pveDevice,
                                          pveRenderer.getSwapChainRenderPass()};
    PveCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));  // camera looks to the center of the cube
    // while the window doesn't want to close, poll window events

    // viewerObject has no model and won't be rendered. It's used to store the camera's current state.
    auto viewerObject = PveGameObject::createGameObject();
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
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

        // the beginFrame function returns a nullptr if the swap chains needs to be recreated
        if (auto commandBuffer = pveRenderer.beginFrame()) {
            pveRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
            pveRenderer.endSwapChainRenderPass(commandBuffer);
            pveRenderer.endFrame();
        }
    }

    // this makes the CPU block until all GPU operations have completed
    vkDeviceWaitIdle(pveDevice.device());
}

// temporary helper function, creates a 1x1x1 cube centered at offset
std::unique_ptr<PveModel> createCubeModel(PveDevice& device, glm::vec3 offset) {
    PveModel::Builder modelBuilder{};

    modelBuilder.vertices = {
        // left face (white)
        {{-.5f, -.5f, -.5f}, colors::white},
        {{-.5f, .5f, .5f}, colors::white},
        {{-.5f, -.5f, .5f}, colors::white},
        {{-.5f, .5f, -.5f}, colors::white},

        // right face (yellow)
        {{.5f, -.5f, -.5f}, colors::yellow},
        {{.5f, .5f, .5f}, colors::yellow},
        {{.5f, -.5f, .5f}, colors::yellow},
        {{.5f, .5f, -.5f}, colors::yellow},

        // top face (orange, remember y axis points down)
        {{-.5f, -.5f, -.5f}, colors::orange},
        {{.5f, -.5f, .5f}, colors::orange},
        {{-.5f, -.5f, .5f}, colors::orange},
        {{.5f, -.5f, -.5f}, colors::orange},

        // bottom face (red)
        {{-.5f, .5f, -.5f}, colors::red},
        {{.5f, .5f, .5f}, colors::red},
        {{-.5f, .5f, .5f}, colors::red},
        {{.5f, .5f, -.5f}, colors::red},

        // nose face (blue)
        {{-.5f, -.5f, .5f}, colors::blue},
        {{.5f, .5f, .5f}, colors::blue},
        {{-.5f, .5f, .5f}, colors::blue},
        {{.5f, -.5f, .5f}, colors::blue},

        // tail face (green)
        {{-.5f, -.5f, -.5f}, colors::green},
        {{.5f, .5f, -.5f}, colors::green},
        {{-.5f, .5f, -.5f}, colors::green},
        {{.5f, -.5f, -.5f}, colors::green},
    };
    for (auto& v : modelBuilder.vertices) {
        v.position += offset;
    }
    modelBuilder.indices = {0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 7, 5, 8, 9, 10, 8, 11, 9,
                            12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};
    return std::make_unique<PveModel>(device, modelBuilder);
}

void FirstApp::loadGameObjects() {
    // std::shared_ptr<PveModel> pveModel = createCubeModel(pveDevice, {.0f, .0f, .0f});
    // std::shared_ptr<PveModel> pveModel = PveModel::createModelFromFile(pveDevice, "models/colored_cube.obj");
    // std::shared_ptr<PveModel> pveModel = PveModel::createModelFromFile(pveDevice, "models/smooth_vase.obj");
    std::shared_ptr<PveModel> pveModel = PveModel::createModelFromFile(pveDevice, "models/flat_vase.obj");
    auto gameObject = PveGameObject::createGameObject();
    gameObject.model = pveModel;
    gameObject.transform.translation = {.0f, .5f, 2.5f};
    gameObject.transform.scale = {3.f, 3.f, 3.f};
    gameObjects.push_back(std::move(gameObject));
}

}  // namespace pve
