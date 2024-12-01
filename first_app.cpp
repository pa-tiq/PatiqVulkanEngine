#include "first_app.hpp"

#include "gravity_physics_system.hpp"
#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS  // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>

namespace pve {

static glm::vec3 red = {1.0f, 0.0f, 0.0f};
static glm::vec3 green = {0.0f, 1.0f, 0.0f};
static glm::vec3 blue = {0.0f, 0.0f, 1.0f};

FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{pveDevice,
                                          pveRenderer.getSwapChainRenderPass()};
    // while the window doesn't want to close, poll window events
    while (!pveWindow.shouldClose()) {
        glfwPollEvents();

        // the beginFrame function returns a nullptr if the swap chains needs to be recreated
        if (auto commandBuffer = pveRenderer.beginFrame()) {
            pveRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
            pveRenderer.endSwapChainRenderPass(commandBuffer);
            pveRenderer.endFrame();
        }
    }

    // this makes the CPU block until all GPU operations have completed
    vkDeviceWaitIdle(pveDevice.device());
}

void FirstApp::runGravityPhisics() {
    // create some models
    std::shared_ptr<PveModel> squareModel = createSquareModel(
        pveDevice,
        {.5f,
         .0f});  // offset model by .5 so rotation occurs at edge rather than center of square
    std::shared_ptr<PveModel> circleModel = createCircleModel(pveDevice, 64);

    // create physics objects
    std::vector<PveGameObject> physicsObjects{};
    auto red = PveGameObject::createGameObject();
    red.transform2d.scale = glm::vec2{.05f};
    red.transform2d.translation = {.5f, .5f};
    red.color = {1.f, 0.f, 0.f};
    red.rigidBody2d.velocity = {-.5f, .0f};
    red.model = circleModel;
    physicsObjects.push_back(std::move(red));
    auto blue = PveGameObject::createGameObject();
    blue.transform2d.scale = glm::vec2{.05f};
    blue.transform2d.translation = {-.45f, -.25f};
    blue.color = {0.f, 0.f, 1.f};
    blue.rigidBody2d.velocity = {.5f, .0f};
    blue.model = circleModel;
    physicsObjects.push_back(std::move(blue));

    // create vector field
    std::vector<PveGameObject> vectorField{};
    int gridCount = 40;
    for (int i = 0; i < gridCount; i++) {
        for (int j = 0; j < gridCount; j++) {
            auto vf = PveGameObject::createGameObject();
            vf.transform2d.scale = glm::vec2(0.005f);
            vf.transform2d.translation = {-1.0f + (i + 0.5f) * 2.0f / gridCount,
                                          -1.0f + (j + 0.5f) * 2.0f / gridCount};
            vf.color = glm::vec3(1.0f);
            vf.model = squareModel;
            vectorField.push_back(std::move(vf));
        }
    }

    GravityPhysicsSystem gravitySystem{0.81f};
    Vec2FieldSystem vecFieldSystem{};

    SimpleRenderSystem simpleRenderSystem{pveDevice,
                                          pveRenderer.getSwapChainRenderPass()};

    while (!pveWindow.shouldClose()) {
        glfwPollEvents();

        if (auto commandBuffer = pveRenderer.beginFrame()) {
            // update systems
            gravitySystem.update(physicsObjects, 1.f / 60, 5);
            vecFieldSystem.update(gravitySystem, physicsObjects, vectorField);

            // render system
            pveRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, physicsObjects);
            simpleRenderSystem.renderGameObjects(commandBuffer, vectorField);
            pveRenderer.endSwapChainRenderPass(commandBuffer);
            pveRenderer.endFrame();
        }
    }

    vkDeviceWaitIdle(pveDevice.device());
}

std::unique_ptr<PveModel> FirstApp::createSquareModel(PveDevice& device,
                                                      glm::vec2 offset) {
    std::vector<PveModel::Vertex> vertices = {
        {{-0.5f, -0.5f}}, {{0.5f, 0.5f}},  {{-0.5f, 0.5f}},
        {{-0.5f, -0.5f}}, {{0.5f, -0.5f}}, {{0.5f, 0.5f}},  //
    };
    for (auto& v : vertices) {
        v.position += offset;
    }
    return std::make_unique<PveModel>(device, vertices);
}

std::unique_ptr<PveModel> FirstApp::createCircleModel(PveDevice& device,
                                                      unsigned int numSides) {
    std::vector<PveModel::Vertex> uniqueVertices{};
    for (int i = 0; i < numSides; i++) {
        float angle = i * glm::two_pi<float>() / numSides;
        uniqueVertices.push_back({{glm::cos(angle), glm::sin(angle)}});
    }
    uniqueVertices.push_back({});  // adds center vertex at 0, 0

    std::vector<PveModel::Vertex> vertices{};
    for (int i = 0; i < numSides; i++) {
        vertices.push_back(uniqueVertices[i]);
        vertices.push_back(uniqueVertices[(i + 1) % numSides]);
        vertices.push_back(uniqueVertices[numSides]);
    }
    return std::make_unique<PveModel>(device, vertices);
}

void FirstApp::sierpinski(std::vector<PveModel::Vertex>& vertices, int depth,
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
    for (auto& color : colors) {
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

void FirstApp::GravityPhisics() {
    std::vector<PveModel::Vertex> vertices{{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                           {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                           {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
    auto pveModel = std::make_shared<PveModel>(pveDevice, vertices);

    auto triangle = PveGameObject::createGameObject();
    triangle.model = pveModel;
    triangle.color = {.1f, .8f, .1f};
    triangle.transform2d.translation.x = .2f;
    triangle.transform2d.scale = {2.f, .5f};
    triangle.transform2d.rotation = .25f * glm::two_pi<float>();

    gameObjects.push_back(std::move(triangle));
}

void FirstApp::loadGameObjects() { GravityPhisics(); }

}  // namespace pve
