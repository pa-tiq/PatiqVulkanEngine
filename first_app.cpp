#include "first_app.hpp"

#include "colors.hpp"
#include "pve_camera.hpp"
#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
#include <vector>

namespace pve {

FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{pveDevice,
                                          pveRenderer.getSwapChainRenderPass()};
    PveCamera camera{};
    // while the window doesn't want to close, poll window events
    while (!pveWindow.shouldClose()) {
        glfwPollEvents();
        float aspect = pveRenderer.getAspectRatio();
        // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
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
    std::vector<PveModel::Vertex> vertices{

        // left face (white)
        {{-.5f, -.5f, -.5f}, colors::white},
        {{-.5f, .5f, .5f}, colors::white},
        {{-.5f, -.5f, .5f}, colors::white},
        {{-.5f, -.5f, -.5f}, colors::white},
        {{-.5f, .5f, -.5f}, colors::white},
        {{-.5f, .5f, .5f}, colors::white},

        // right face (yellow)
        {{.5f, -.5f, -.5f}, colors::yellow},
        {{.5f, .5f, .5f}, colors::yellow},
        {{.5f, -.5f, .5f}, colors::yellow},
        {{.5f, -.5f, -.5f}, colors::yellow},
        {{.5f, .5f, -.5f}, colors::yellow},
        {{.5f, .5f, .5f}, colors::yellow},

        // top face (orange, remember y axis points down)
        {{-.5f, -.5f, -.5f}, colors::orange},
        {{.5f, -.5f, .5f}, colors::orange},
        {{-.5f, -.5f, .5f}, colors::orange},
        {{-.5f, -.5f, -.5f}, colors::orange},
        {{.5f, -.5f, -.5f}, colors::orange},
        {{.5f, -.5f, .5f}, colors::orange},

        // bottom face (red)
        {{-.5f, .5f, -.5f}, colors::red},
        {{.5f, .5f, .5f}, colors::red},
        {{-.5f, .5f, .5f}, colors::red},
        {{-.5f, .5f, -.5f}, colors::red},
        {{.5f, .5f, -.5f}, colors::red},
        {{.5f, .5f, .5f}, colors::red},

        // nose face (blue)
        {{-.5f, -.5f, 0.5f}, colors::blue},
        {{.5f, .5f, 0.5f}, colors::blue},
        {{-.5f, .5f, 0.5f}, colors::blue},
        {{-.5f, -.5f, 0.5f}, colors::blue},
        {{.5f, -.5f, 0.5f}, colors::blue},
        {{.5f, .5f, 0.5f}, colors::blue},

        // tail face (green)
        {{-.5f, -.5f, -0.5f}, colors::green},
        {{.5f, .5f, -0.5f}, colors::green},
        {{-.5f, .5f, -0.5f}, colors::green},
        {{-.5f, -.5f, -0.5f}, colors::green},
        {{.5f, -.5f, -0.5f}, colors::green},
        {{.5f, .5f, -0.5f}, colors::green},

    };
    for (auto& v : vertices) {
        v.position += offset;
    }
    return std::make_unique<PveModel>(device, vertices);
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<PveModel> pveModel = createCubeModel(pveDevice, {.0f, .0f, .0f});
    auto cube = PveGameObject::createGameObject();
    cube.model = pveModel;
    cube.transform.translation = {.0f, .0f, 2.5f};
    cube.transform.scale = {.5f, .5f, .5f};
    gameObjects.push_back(std::move(cube));
}

}  // namespace pve
