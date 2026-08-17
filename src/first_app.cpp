#include "first_app.hpp"

#include "constants/colors.hpp"
#include "controllers/keyboard_movement_controller.hpp"
#include "pve/pve_buffer.hpp"
#include "pve/pve_camera.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/ui_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS  // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pve {

float MAX_FRAME_TIME = 0.1f;

FirstApp::FirstApp() : gameState(GameStateManager::getInstance()), i18n(I18n::getInstance()) {
    globalPool = PveDescriptorPool::Builder(pveDevice)
                     .setMaxSets(PveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  PveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  PveSwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();
    shadowMapSystem = std::make_unique<ShadowMapSystem>(pveDevice);

    loadGameObjects();
    
    // Set default language to Portuguese
    i18n.setLanguage("pt_br");
}

FirstApp::~FirstApp() {}

void FirstApp::run() {
    std::vector<std::unique_ptr<PveBuffer>> uboBuffers(
        PveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<PveBuffer>(
            pveDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = PveDescriptorSetLayout::Builder(pveDevice)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           VK_SHADER_STAGE_ALL_GRAPHICS)
                               .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                           VK_SHADER_STAGE_FRAGMENT_BIT)
                               .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(PveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = shadowMapSystem->getShadowMapSampler();
        imageInfo.imageView = shadowMapSystem->getShadowMapView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        PveDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo)
            .build(globalDescriptorSets[i]);
    }

    SimpleRenderSystem simpleRenderSystem{pveDevice, pveRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};

    PointLightSystem pointLightSystem{pveDevice, pveRenderer.getSwapChainRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};
    
    uiRenderSystem = std::make_unique<UIRenderSystem>(pveDevice, pveRenderer.getSwapChainRenderPass());
    
    PveCamera camera{};
    camera.setViewTarget(
        glm::vec3(-5.f, -5.f, 5.f),
        glm::vec3(1.f, 1.f, 2.5f));  // camera looks to the center of the cube
    // while the window doesn't want to close, poll window events

    // viewerObject has no model and won't be rendered. It's used to store the camera's current state.
    viewerObject = std::make_unique<PveGameObject>(PveGameObject::createGameObject());
    viewerObject->transform.translation.z = -2.5f;
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!pveWindow.shouldClose()) {
        glfwPollEvents();

        // Track mouse position
        glfwGetCursorPos(pveWindow.getGLFWWindow(), &mouseX, &mouseY);

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(
                              newTime - currentTime)
                              .count();
        currentTime = newTime;
        frameTime = glm::min(frameTime, MAX_FRAME_TIME);

        handleInput(pveWindow.getGLFWWindow(), frameTime);

        if (gameState.isPlaying()) {
            cameraController.moveInPlaneXZ(pveWindow.getGLFWWindow(), frameTime,
                                           *viewerObject);
            camera.setViewYXZ(viewerObject->transform.translation,
                              viewerObject->transform.rotation);
            for (auto& kv : gameObjects) {
                auto& obj = kv.second;

                if (obj.name == "cube") {
                    obj.transform.rotation.y =
                        glm::mod(obj.transform.rotation.y + 0.001f, glm::two_pi<float>());
                    obj.transform.rotation.x =
                        glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());
                }
            }
        }

        float aspect = pveRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        // the beginFrame function returns a nullptr if the swap chains needs to be recreated
        if (auto commandBuffer = pveRenderer.beginFrame()) {
            int frameIndex = pveRenderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex,
                                frameTime,
                                commandBuffer,
                                camera,
                                globalDescriptorSets[frameIndex],
                                gameObjects};

            if (gameState.isPlaying()) {
                // Update shadow map before main rendering
                pointLightSystem.updateShadowMap(frameInfo);

                // prepare and update objects in memory
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                pointLightSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();
            }

            // render - record draw calls
            pveRenderer.beginSwapChainRenderPass(commandBuffer);

            if (gameState.isPlaying() || gameState.isPaused()) {
                // order here matters
                simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
            }

            if (gameState.isPaused()) {
                renderPauseModal(commandBuffer);
            } else if (gameState.isInMenu()) {
                renderMenu(commandBuffer);
            }

            pveRenderer.endSwapChainRenderPass(commandBuffer);
            pveRenderer.endFrame();
        }
    }

    // this makes the CPU block until all GPU operations have completed
    vkDeviceWaitIdle(pveDevice.device());
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<PveModel> pveModel =
        PveModel::createModelFromFile(pveDevice, "models/cube.obj");
    auto cube = PveGameObject::createGameObject();
    cube.model = pveModel;
    cube.name = "cube";
    cube.transform.translation = {-1.0f, -1.0f, 0.f};
    cube.transform.scale = {.3f, .3f, .3f};
    gameObjects.emplace(cube.getId(), std::move(cube));

    pveModel = PveModel::createModelFromFile(pveDevice, "models/quad.obj");
    auto floor = PveGameObject::createGameObject();
    floor.model = pveModel;
    floor.name = "floor";
    floor.transform.translation = {0.f, 0.0f, 0.f};
    floor.transform.scale = {3.f, 1.f, 3.f};
    gameObjects.emplace(floor.getId(), std::move(floor));

    pveModel = PveModel::createModelFromFile(pveDevice, "models/flat_vase.obj");
    auto flatVase = PveGameObject::createGameObject();
    flatVase.model = pveModel;
    flatVase.name = "flatVase";
    flatVase.transform.translation = {1.0f, 0.f, 0.f};
    flatVase.transform.scale = {3.f, 3.f, 3.f};
    gameObjects.emplace(flatVase.getId(), std::move(flatVase));

    pveModel = PveModel::createModelFromFile(pveDevice, "models/smooth_vase.obj");
    auto smoothVase = PveGameObject::createGameObject();
    smoothVase.model = pveModel;
    smoothVase.name = "smoothVase";
    smoothVase.transform.translation = {2.0f, 0.f, 0.f};
    smoothVase.transform.scale = {3.f, 3.f, 3.f};
    gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

    std::vector<glm::vec3> lightColors{{1.f, .1f, .1f}, {.1f, .1f, 1.f}, {.1f, 1.f, .1f},
                                       {1.f, 1.f, .1f}, {.1f, 1.f, 1.f}, {1.f, 1.f, 1.f}};

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = PveGameObject::makePointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight =
            glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(),
                        glm::vec3(0.f, -1.f, 0.f));
        pointLight.transform.translation =
            glm::vec3(rotateLight * glm::vec4(-1.f, -2.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
}

void FirstApp::handleInput(GLFWwindow* window, float frameTime) {
    static bool escPressed = false;
    static bool mousePressed = false;
    static bool upPressed = false;
    static bool downPressed = false;
    static bool enterPressed = false;
    
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!escPressed) {
            escPressed = true;
            
            if (gameState.isInMenu()) {
                // ESC in menu closes the game
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } else if (gameState.isPlaying()) {
                // ESC in play shows pause modal
                gameState.setState(GameState::PAUSED);
                selectedMenuIndex = 0;
            } else if (gameState.isPaused()) {
                // ESC in pause modal resumes game
                gameState.setState(GameState::PLAYING);
            }
        }
    } else {
        escPressed = false;
    }
    
    // Handle keyboard navigation for menus
    if (gameState.isInMenu() || gameState.isPaused()) {
        int numButtons = gameState.isInMenu() ? 2 : 2;
        
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            if (!upPressed) {
                upPressed = true;
                selectedMenuIndex = (selectedMenuIndex - 1 + numButtons) % numButtons;
            }
        } else {
            upPressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            if (!downPressed) {
                downPressed = true;
                selectedMenuIndex = (selectedMenuIndex + 1) % numButtons;
            }
        } else {
            downPressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (!enterPressed) {
                enterPressed = true;
                
                if (gameState.isInMenu()) {
                    if (selectedMenuIndex == 0) {
                        gameState.setState(GameState::PLAYING);
                    } else if (selectedMenuIndex == 1) {
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }
                } else if (gameState.isPaused()) {
                    if (selectedMenuIndex == 0) {
                        gameState.setState(GameState::MENU);
                        viewerObject->transform.translation = glm::vec3(0.f);
                        viewerObject->transform.rotation = glm::vec3(0.f);
                        selectedMenuIndex = 0;
                    } else if (selectedMenuIndex == 1) {
                        gameState.setState(GameState::PLAYING);
                    }
                }
            }
        } else {
            enterPressed = false;
        }
    }
    
    // Handle mouse clicks for UI
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mousePressed) {
            mousePressed = true;
            
            if (gameState.isInMenu()) {
                // Check menu button clicks
                float playButtonX = WIDTH / 2.0f - 100.0f;
                float playButtonY = HEIGHT / 2.0f - 50.0f;
                float exitButtonX = WIDTH / 2.0f - 100.0f;
                float exitButtonY = HEIGHT / 2.0f + 50.0f;
                
                if (mouseX >= playButtonX && mouseX <= playButtonX + 200.0f &&
                    mouseY >= playButtonY && mouseY <= playButtonY + 50.0f) {
                    gameState.setState(GameState::PLAYING);
                }
                
                if (mouseX >= exitButtonX && mouseX <= exitButtonX + 200.0f &&
                    mouseY >= exitButtonY && mouseY <= exitButtonY + 50.0f) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            } else if (gameState.isPaused()) {
                // Check pause modal button clicks
                float menuButtonX = WIDTH / 2.0f - 150.0f;
                float menuButtonY = HEIGHT / 2.0f + 50.0f;
                float resumeButtonX = WIDTH / 2.0f + 50.0f;
                float resumeButtonY = HEIGHT / 2.0f + 50.0f;
                
                if (mouseX >= menuButtonX && mouseX <= menuButtonX + 100.0f &&
                    mouseY >= menuButtonY && mouseY <= menuButtonY + 50.0f) {
                    gameState.setState(GameState::MENU);
                    viewerObject->transform.translation = glm::vec3(0.f);
                    viewerObject->transform.rotation = glm::vec3(0.f);
                }
                
                if (mouseX >= resumeButtonX && mouseX <= resumeButtonX + 100.0f &&
                    mouseY >= resumeButtonY && mouseY <= resumeButtonY + 50.0f) {
                    gameState.setState(GameState::PLAYING);
                }
            }
        }
    } else {
        mousePressed = false;
    }
}

void FirstApp::renderMenu(VkCommandBuffer commandBuffer) {
    PveCamera uiCamera{};
    FrameInfo frameInfo{0, 0.0f, commandBuffer, uiCamera, VK_NULL_HANDLE, gameObjects,
                        static_cast<float>(WIDTH), static_cast<float>(HEIGHT)};
    
    // Hacker green color
    glm::vec4 hackerGreen = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 hackerGreenDark = glm::vec4(0.0f, 0.4f, 0.0f, 1.0f);
    glm::vec4 hackerGreenHover = glm::vec4(0.0f, 0.7f, 0.0f, 1.0f);
    glm::vec4 selectedColor = glm::vec4(0.0f, 0.6f, 0.0f, 1.0f);
    
    // Create menu buttons
    std::vector<UIButton> buttons;
    
    // Play button
    UIButton playButton;
    playButton.position = glm::vec2(WIDTH / 2.0f - 100.0f, HEIGHT / 2.0f - 50.0f);
    playButton.size = glm::vec2(200.0f, 50.0f);
    playButton.color = (selectedMenuIndex == 0) ? selectedColor : hackerGreenDark;
    playButton.hoverColor = hackerGreenHover;
    playButton.text = i18n.get("menu.play");
    playButton.id = 1;
    buttons.push_back(playButton);
    
    // Exit button
    UIButton exitButton;
    exitButton.position = glm::vec2(WIDTH / 2.0f - 100.0f, HEIGHT / 2.0f + 50.0f);
    exitButton.size = glm::vec2(200.0f, 50.0f);
    exitButton.color = (selectedMenuIndex == 1) ? selectedColor : hackerGreenDark;
    exitButton.hoverColor = hackerGreenHover;
    exitButton.text = i18n.get("menu.exit");
    exitButton.id = 2;
    buttons.push_back(exitButton);
    
    // Render buttons (text labels are rendered inside renderGameObjects)
    uiRenderSystem->renderGameObjects(frameInfo, buttons, static_cast<int>(mouseX), static_cast<int>(mouseY));
    
    // Render title text in hacker green
    uiRenderSystem->renderText(frameInfo, i18n.get("menu.title"), 
                              glm::vec2(WIDTH / 2.0f - 150.0f, HEIGHT / 2.0f - 150.0f),
                              hackerGreen);
}

void FirstApp::renderPauseModal(VkCommandBuffer commandBuffer) {
    PveCamera uiCamera{};
    FrameInfo frameInfo{0, 0.0f, commandBuffer, uiCamera, VK_NULL_HANDLE, gameObjects,
                        static_cast<float>(WIDTH), static_cast<float>(HEIGHT)};
    
    // Hacker green color
    glm::vec4 hackerGreen = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 hackerGreenDark = glm::vec4(0.0f, 0.4f, 0.0f, 1.0f);
    glm::vec4 hackerGreenHover = glm::vec4(0.0f, 0.7f, 0.0f, 1.0f);
    glm::vec4 selectedColor = glm::vec4(0.0f, 0.6f, 0.0f, 1.0f);
    
    // Create pause modal buttons
    std::vector<UIButton> buttons;
    
    // Return to menu button
    UIButton menuButton;
    menuButton.position = glm::vec2(WIDTH / 2.0f - 150.0f, HEIGHT / 2.0f + 50.0f);
    menuButton.size = glm::vec2(100.0f, 50.0f);
    menuButton.color = (selectedMenuIndex == 0) ? selectedColor : hackerGreenDark;
    menuButton.hoverColor = hackerGreenHover;
    menuButton.text = i18n.get("pause.return_to_menu");
    menuButton.id = 1;
    buttons.push_back(menuButton);
    
    // Resume button
    UIButton resumeButton;
    resumeButton.position = glm::vec2(WIDTH / 2.0f + 50.0f, HEIGHT / 2.0f + 50.0f);
    resumeButton.size = glm::vec2(100.0f, 50.0f);
    resumeButton.color = (selectedMenuIndex == 1) ? selectedColor : hackerGreenDark;
    resumeButton.hoverColor = hackerGreenHover;
    resumeButton.text = i18n.get("pause.resume");
    resumeButton.id = 2;
    buttons.push_back(resumeButton);
    
    // Render buttons
    uiRenderSystem->renderGameObjects(frameInfo, buttons, static_cast<int>(mouseX), static_cast<int>(mouseY));
    
    // Render title text in hacker green
    uiRenderSystem->renderText(frameInfo, i18n.get("pause.title"), 
                              glm::vec2(WIDTH / 2.0f - 100.0f, HEIGHT / 2.0f - 50.0f),
                              hackerGreen);
}

}  // namespace pve
