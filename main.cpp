#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#include "loadModel/model.h"
#include "shader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const int SSDO_KERNEL_SIZE = 32;

const unsigned int SHADOW_SIZE = 1024;
const float SHADOW_NEAR = 0.05f;
const float SHADOW_FAR = 8.0f;
const unsigned int RSM_SIZE = 1024;
const float RSM_NEAR = 0.05f;
const float RSM_FAR = 8.0f;

Camera camera(glm::vec3(0.0f, -0.05f, 2.35f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -5.0f);

float lastX = SCR_WIDTH * 0.5f;
float lastY = SCR_HEIGHT * 0.5f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPosWorld(0.0f, 0.78f, -0.1f);
glm::vec3 lightBaseColor(1.0f, 1.0f, 1.0f);
float pointLightIntensity = 3.0f;
float ambientStrength = 0.02f;

float ssdoRadius = 0.32f;
float ssdoBias = 0.02f;
float ssdoDetailStrength = 2.0f;
float rsmIntensity = 0.95f;
float rsmSampleRadius = 0.20f;
float rsmDepthBias = 0.0015f;
bool enableSSDODetail = true;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    camera.mouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    camera.mouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.keyboardMovement(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.keyboardMovement(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.keyboardMovement(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.keyboardMovement(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.keyboardMovement(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        camera.keyboardMovement(DOWN, deltaTime);
    }

    float lightStep = 0.9f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        lightPosWorld.z -= lightStep;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        lightPosWorld.z += lightStep;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        lightPosWorld.x -= lightStep;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        lightPosWorld.x += lightStep;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
        lightPosWorld.y += lightStep;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
        lightPosWorld.y -= lightStep;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        ssdoRadius = std::min(ssdoRadius + 0.2f * deltaTime, 0.8f);
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        ssdoRadius = std::max(ssdoRadius - 0.2f * deltaTime, 0.1f);
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        rsmSampleRadius = std::min(rsmSampleRadius + 0.2f * deltaTime, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        rsmSampleRadius = std::max(rsmSampleRadius - 0.2f * deltaTime, 0.03f);
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        ssdoDetailStrength = std::min(ssdoDetailStrength + 1.0f * deltaTime, 4.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        ssdoDetailStrength = std::max(ssdoDetailStrength - 1.0f * deltaTime, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        pointLightIntensity = std::min(pointLightIntensity + 4.0f * deltaTime, 25.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        pointLightIntensity = std::max(pointLightIntensity - 4.0f * deltaTime, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        ambientStrength = std::min(ambientStrength + 0.8f * deltaTime, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        ambientStrength = std::max(ambientStrength - 0.8f * deltaTime, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        rsmIntensity = std::min(rsmIntensity + 0.6f * deltaTime, 3.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        rsmIntensity = std::max(rsmIntensity - 0.6f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        rsmDepthBias = std::min(rsmDepthBias + 0.004f * deltaTime, 0.01f);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        rsmDepthBias = std::max(rsmDepthBias - 0.004f * deltaTime, 0.0f);
    }

    static bool bKeyLatch = false;
    bool bDown = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
    if (bDown && !bKeyLatch) {
        enableSSDODetail = !enableSSDODetail;
        std::cout << "[TOGGLE] Compare mode: " << (enableSSDODetail ? "RSM + SSDO detail" : "RSM only (no SSDO detail)") << std::endl;
    }
    bKeyLatch = bDown;
}

void renderQuad() {
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO = 0;

    if (quadVAO == 0) {
        const float quadVertices[] = {
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void renderCube() {
    static unsigned int cubeVAO = 0;
    static unsigned int cubeVBO = 0;

    if (cubeVAO == 0) {
        const float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
             0.5f, 0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,  -0.5f, 0.5f, -0.5f,
            -0.5f, -0.5f, 0.5f,   0.5f, -0.5f, 0.5f,   0.5f, 0.5f, 0.5f,
             0.5f, 0.5f, 0.5f,   -0.5f, 0.5f, 0.5f,   -0.5f, -0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f,   -0.5f, 0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
             0.5f, 0.5f, 0.5f,    0.5f, -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,   0.5f, -0.5f, 0.5f,
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, 0.5f,
             0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f,
            -0.5f, 0.5f, -0.5f,   0.5f, 0.5f, 0.5f,    0.5f, 0.5f, -0.5f,
             0.5f, 0.5f, 0.5f,   -0.5f, 0.5f, -0.5f,  -0.5f, 0.5f, 0.5f,
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int createColorAttachment(unsigned int width, unsigned int height, GLenum internalFormat, GLenum format, GLenum type) {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<int>(width), static_cast<int>(height), 0, format, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

unsigned int createColorAttachmentLinear(unsigned int width, unsigned int height, GLenum internalFormat, GLenum format, GLenum type) {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<int>(width), static_cast<int>(height), 0, format, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

unsigned int createDepthTexture(unsigned int width, unsigned int height) {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, static_cast<int>(width), static_cast<int>(height), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void checkFramebufferComplete(const std::string& label) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ERROR] Framebuffer incomplete: " << label << std::endl;
    }
}

std::string resolveCornellModelDir() {
    const std::array<std::string, 4> candidates = {
        "../model/cornell_box",
        "./model/cornell_box",
        "../../model/cornell_box",
        "../../../model/cornell_box",
    };

    for (const auto& dir : candidates) {
        if (std::filesystem::exists(dir + "/cornell_box.obj")) {
            return dir;
        }
    }

    std::cerr << "[WARN] cornell_box.obj not found in fallback paths, using default: " << candidates[0] << std::endl;
    return candidates[0];
}

std::array<glm::mat4, 6> buildPointShadowTransforms(const glm::vec3& lightPos) {
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, SHADOW_NEAR, SHADOW_FAR);
    return {
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f),  glm::vec3(0.0f, 0.0f, 1.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    };
}

glm::mat4 buildRsmLightVP(const glm::vec3& lightPos) {
    const glm::vec3 sceneCenter(0.0f, -0.72f, -0.25f);
    glm::vec3 toCenter = sceneCenter - lightPos;
    if (glm::length(toCenter) < 0.001f) {
        toCenter = glm::vec3(0.0f, -1.0f, 0.0f);
    }

    const glm::vec3 up = (std::abs(toCenter.y) > 0.9f) ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + glm::normalize(toCenter), up);
    glm::mat4 lightProj = glm::perspective(glm::radians(120.0f), 1.0f, RSM_NEAR, RSM_FAR);
    return lightProj * lightView;
}

std::string resolveAnimalModelDir() {
    const std::array<std::string, 4> candidates = {
        "../model/animal",
        "./model/animal",
        "../../model/animal",
        "../../../model/animal",
    };

    for (const auto& dir : candidates) {
        if (std::filesystem::exists(dir + "/stanford_bunny.obj") &&
            std::filesystem::exists(dir + "/armadillo.obj")) {
            return dir;
        }
    }

    std::cerr << "[WARN] animal models not found in fallback paths, using default: " << candidates[0] << std::endl;
    return candidates[0];
}

void drawScene(Shader& shader, Model& room, Model& bunny, Model& armadillo) {
    glm::mat4 roomModel = glm::mat4(1.0f);
    roomModel = glm::translate(roomModel, glm::vec3(0.98f, -1.0f, 0.95f));
    roomModel = glm::rotate(roomModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    roomModel = glm::scale(roomModel, glm::vec3(0.0035f));
    shader.setMat4("model", roomModel);
    shader.setVec3("uFallbackColor", glm::vec3(0.74f));
    room.Draw(shader);

    // Replace the previous box with a bunny to emphasize contact shadows and detail GI.
    // Rotate 180 degrees to face the camera and enlarge for clearer SSDO contrast.
    glm::mat4 bunnyModel = glm::mat4(1.0f);
    bunnyModel = glm::translate(bunnyModel, glm::vec3(-0.46f, -1.26f, -0.36f));
    bunnyModel = glm::rotate(bunnyModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    bunnyModel = glm::scale(bunnyModel, glm::vec3(7.8f));
    shader.setMat4("model", bunnyModel);
    shader.setVec3("uFallbackColor", glm::vec3(0.70f, 0.70f, 0.68f));
    bunny.Draw(shader);

    // Use a second animal model so B-toggle comparison is obvious on curved geometry.
    // Rotate 180 degrees to face the camera and enlarge for clearer contact shadows.
    glm::mat4 armadilloModel = glm::mat4(1.0f);
    armadilloModel = glm::translate(armadilloModel, glm::vec3(0.38f, -0.58f, -0.22f));
    armadilloModel = glm::rotate(armadilloModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    armadilloModel = glm::scale(armadilloModel, glm::vec3(0.0078f));
    shader.setMat4("model", armadilloModel);
    shader.setVec3("uFallbackColor", glm::vec3(0.73f, 0.71f, 0.69f));
    armadillo.Draw(shader);
}

std::vector<glm::vec3> buildKernel() {
    std::vector<glm::vec3> kernel;
    kernel.reserve(SSDO_KERNEL_SIZE);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    for (int i = 0; i < SSDO_KERNEL_SIZE; ++i) {
        glm::vec3 sample(
            randomFloats(rng) * 2.0f - 1.0f,
            randomFloats(rng) * 2.0f - 1.0f,
            randomFloats(rng)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(rng);

        float scale = static_cast<float>(i) / static_cast<float>(SSDO_KERNEL_SIZE);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;

        kernel.push_back(sample);
    }

    return kernel;
}

unsigned int buildNoiseTexture() {
    std::vector<glm::vec3> noise;
    noise.reserve(16);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    for (int i = 0; i < 16; ++i) {
        noise.emplace_back(
            randomFloats(rng) * 2.0f - 1.0f,
            randomFloats(rng) * 2.0f - 1.0f,
            0.0f
        );
    }

    unsigned int noiseTex = 0;
    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    return noiseTex;
}

} // namespace

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SSDO Depth Peeling", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader geometryShader("./shader/ssdo/geometry.vert", "./shader/ssdo/geometry.frag");
    Shader peelShader("./shader/ssdo/depth_peel.vert", "./shader/ssdo/depth_peel.frag");
    Shader rsmCaptureShader("./shader/ssdo/rsm_capture.vert", "./shader/ssdo/rsm_capture.frag");
    Shader rsmGatherShader("./shader/ssdo/screen_quad.vert", "./shader/ssdo/rsm_gather.frag");
    Shader rsmBlurShader("./shader/ssdo/screen_quad.vert", "./shader/ssdo/rsm_blur.frag");
    Shader ssdoShader("./shader/ssdo/screen_quad.vert", "./shader/ssdo/ssdo.frag");
    Shader blurShader("./shader/ssdo/screen_quad.vert", "./shader/ssdo/blur.frag");
    Shader compositeShader("./shader/ssdo/screen_quad.vert", "./shader/ssdo/composite.frag");
    Shader pointShadowShader("./shader/ssdo/point_shadow.vert", "./shader/ssdo/point_shadow.frag");
    Shader lightShader("./shader/light/light.vert", "./shader/light/light.frag");

    const std::string modelDir = resolveCornellModelDir();
    const std::string animalDir = resolveAnimalModelDir();
    std::string roomPath = modelDir + "/cornell_box.obj";
    std::string bunnyPath = animalDir + "/stanford_bunny.obj";
    std::string armadilloPath = animalDir + "/armadillo.obj";

    Model roomModel(const_cast<char*>(roomPath.c_str()));
    Model bunnyModel(const_cast<char*>(bunnyPath.c_str()));
    Model armadilloModel(const_cast<char*>(armadilloPath.c_str()));

    std::cout
        << "[Controls] I/K point-light intensity, U/J ambient, O/P RSM intensity, N/M RSM depth bias\n"
        << "[Controls] R/F SSDO radius, T/G RSM sample radius, Y/H SSDO detail strength, B compare toggle"
        << std::endl;

    unsigned int pointShadowFBO = 0;
    unsigned int pointShadowCube = 0;
    glGenFramebuffers(1, &pointShadowFBO);
    glGenTextures(1, &pointShadowCube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCube);
    for (unsigned int face = 0; face < 6; ++face) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,
            GL_DEPTH_COMPONENT,
            SHADOW_SIZE,
            SHADOW_SIZE,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, pointShadowCube, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    checkFramebufferComplete("pointShadowFBO");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int gBuffer = 0;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    unsigned int gPosition = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
    unsigned int gNormal = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
    unsigned int gAlbedo = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    unsigned int gDepth = createDepthTexture(SCR_WIDTH, SCR_HEIGHT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);
    {
        const unsigned int attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, attachments);
    }
    checkFramebufferComplete("gBuffer");

    unsigned int peelFBO = 0;
    glGenFramebuffers(1, &peelFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, peelFBO);

    unsigned int peelPosition = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
    unsigned int peelNormal = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
    unsigned int peelAlbedo = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    unsigned int peelDepth = createDepthTexture(SCR_WIDTH, SCR_HEIGHT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, peelPosition, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, peelNormal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, peelAlbedo, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, peelDepth, 0);
    {
        const unsigned int attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, attachments);
    }
    checkFramebufferComplete("peelFBO");

    unsigned int rsmFBO = 0;
    unsigned int rsmFluxTex = 0;
    unsigned int rsmNormalTex = 0;
    unsigned int rsmDepthTex = 0;
    glGenFramebuffers(1, &rsmFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rsmFBO);

    rsmFluxTex = createColorAttachmentLinear(RSM_SIZE, RSM_SIZE, GL_RGB16F, GL_RGB, GL_FLOAT);
    rsmNormalTex = createColorAttachmentLinear(RSM_SIZE, RSM_SIZE, GL_RGB16F, GL_RGB, GL_FLOAT);
    rsmDepthTex = createDepthTexture(RSM_SIZE, RSM_SIZE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rsmFluxTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rsmNormalTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rsmDepthTex, 0);
    {
        const unsigned int attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);
    }
    checkFramebufferComplete("rsmFBO");

    unsigned int rsmGiFBO = 0;
    unsigned int rsmGiTex = 0;
    glGenFramebuffers(1, &rsmGiFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rsmGiFBO);
    rsmGiTex = createColorAttachmentLinear(SCR_WIDTH, SCR_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rsmGiTex, 0);
    checkFramebufferComplete("rsmGiFBO");

    unsigned int rsmSmoothFBO = 0;
    unsigned int rsmSmoothTex = 0;
    glGenFramebuffers(1, &rsmSmoothFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rsmSmoothFBO);
    rsmSmoothTex = createColorAttachmentLinear(SCR_WIDTH, SCR_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rsmSmoothTex, 0);
    checkFramebufferComplete("rsmSmoothFBO");

    unsigned int ssdoFBO = 0;
    unsigned int ssdoColor = 0;
    glGenFramebuffers(1, &ssdoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssdoFBO);
    ssdoColor = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColor, 0);
    checkFramebufferComplete("ssdoFBO");

    unsigned int blurFBO = 0;
    unsigned int blurColor = 0;
    glGenFramebuffers(1, &blurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
    blurColor = createColorAttachment(SCR_WIDTH, SCR_HEIGHT, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurColor, 0);
    checkFramebufferComplete("blurFBO");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const std::vector<glm::vec3> kernel = buildKernel();
    const unsigned int noiseTex = buildNoiseTexture();

    rsmGatherShader.use();
    rsmGatherShader.setInt("gPosition", 0);
    rsmGatherShader.setInt("gNormal", 1);
    rsmGatherShader.setInt("gDepth", 2);
    rsmGatherShader.setInt("gAlbedo", 3);
    rsmGatherShader.setInt("rsmFlux", 4);
    rsmGatherShader.setInt("rsmNormal", 5);
    rsmGatherShader.setInt("rsmDepth", 6);

    rsmBlurShader.use();
    rsmBlurShader.setInt("rsmInput", 0);
    rsmBlurShader.setInt("depthTex", 1);

    ssdoShader.use();
    ssdoShader.setInt("gPosition", 0);
    ssdoShader.setInt("gNormal", 1);
    ssdoShader.setInt("gAlbedo", 2);
    ssdoShader.setInt("gDepth", 3);
    ssdoShader.setInt("peelPosition", 4);
    ssdoShader.setInt("peelNormal", 5);
    ssdoShader.setInt("peelAlbedo", 6);
    ssdoShader.setInt("peelDepth", 7);
    ssdoShader.setInt("texNoise", 8);
    for (int i = 0; i < SSDO_KERNEL_SIZE; ++i) {
        ssdoShader.setVec3("uKernel[" + std::to_string(i) + "]", kernel[i]);
    }

    blurShader.use();
    blurShader.setInt("ssdoInput", 0);
    blurShader.setInt("depthTex", 1);

    compositeShader.use();
    compositeShader.setInt("gPosition", 0);
    compositeShader.setInt("gNormal", 1);
    compositeShader.setInt("gAlbedo", 2);
    compositeShader.setInt("gDepth", 3);
    compositeShader.setInt("ssdoTex", 4);
    compositeShader.setInt("rsmGiTex", 5);
    compositeShader.setInt("shadowCube", 6);
    compositeShader.setInt("uEnableSSDODetail", 1);

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glm::mat4 view = camera.getLookAt();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT),
            0.05f,
            20.0f
        );

        // Pass 0: point-light shadow cubemap generation.
        const auto shadowTransforms = buildPointShadowTransforms(lightPosWorld);
        glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
        glEnable(GL_DEPTH_TEST);

        pointShadowShader.use();
        pointShadowShader.setVec3("lightPos", lightPosWorld);
        pointShadowShader.setFloat("farPlane", SHADOW_FAR);

        for (unsigned int face = 0; face < 6; ++face) {
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                pointShadowCube,
                0
            );
            glClear(GL_DEPTH_BUFFER_BIT);
            pointShadowShader.setMat4("lightVP", shadowTransforms[face]);
            drawScene(pointShadowShader, roomModel, bunnyModel, armadilloModel);
        }

        const glm::mat4 lightVP = buildRsmLightVP(lightPosWorld);

        // Pass 1: primary visible layer in G-buffer.
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        geometryShader.use();
        geometryShader.setMat4("view", view);
        geometryShader.setMat4("projection", projection);
        drawScene(geometryShader, roomModel, bunnyModel, armadilloModel);

        // Pass 2: depth peeling for the second layer used by SSDO detail recovery.
        glBindFramebuffer(GL_FRAMEBUFFER, peelFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        peelShader.use();
        peelShader.setMat4("view", view);
        peelShader.setMat4("projection", projection);
        peelShader.setFloat("uDepthEpsilon", 0.0002f);
        glUniform2f(glGetUniformLocation(peelShader.ID, "uScreenSize"), static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT));

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gDepth);
        peelShader.setInt("uFrontDepth", 3);

        drawScene(peelShader, roomModel, bunnyModel, armadilloModel);

        // Pass 3: coarse GI source generation (RSM capture in light space).
        glBindFramebuffer(GL_FRAMEBUFFER, rsmFBO);
        glViewport(0, 0, RSM_SIZE, RSM_SIZE);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rsmCaptureShader.use();
        rsmCaptureShader.setMat4("lightVP", lightVP);
        rsmCaptureShader.setVec3("lightPosWS", lightPosWorld);
        rsmCaptureShader.setVec3("lightColor", lightBaseColor * pointLightIntensity);
        drawScene(rsmCaptureShader, roomModel, bunnyModel, armadilloModel);

        // Pass 4: coarse GI reconstruction from RSM (Chapter 5 first stage).
        glBindFramebuffer(GL_FRAMEBUFFER, rsmGiFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        rsmGatherShader.use();
        rsmGatherShader.setMat4("invView", glm::inverse(view));
        rsmGatherShader.setMat4("lightVP", lightVP);
        rsmGatherShader.setMat4("invLightVP", glm::inverse(lightVP));
        rsmGatherShader.setFloat("sampleRadius", rsmSampleRadius);
        rsmGatherShader.setFloat("depthBias", rsmDepthBias);
        rsmGatherShader.setInt("sampleCount", 96);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gDepth);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, rsmFluxTex);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, rsmNormalTex);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, rsmDepthTex);

        renderQuad();

        // Pass 4.5: blur RSM coarse GI to remove blocky artifacts.
        glBindFramebuffer(GL_FRAMEBUFFER, rsmSmoothFBO);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        rsmBlurShader.use();
        glUniform2f(glGetUniformLocation(rsmBlurShader.ID, "texelSize"), 1.0f / static_cast<float>(SCR_WIDTH), 1.0f / static_cast<float>(SCR_HEIGHT));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rsmGiTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gDepth);

        renderQuad();

        // Pass 5: SSDO detail shadow + local detail bounce (Chapter 5 second stage).
        glBindFramebuffer(GL_FRAMEBUFFER, ssdoFBO);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        ssdoShader.use();
        ssdoShader.setMat4("projection", projection);
        ssdoShader.setMat4("view", view);
        ssdoShader.setMat4("invView", glm::inverse(view));
        ssdoShader.setVec3("lightPosWS", lightPosWorld);
        ssdoShader.setFloat("radius", ssdoRadius);
        ssdoShader.setFloat("bias", ssdoBias);
        ssdoShader.setFloat("shadowMapTexel", 1.0f / static_cast<float>(SHADOW_SIZE));
        glUniform2f(glGetUniformLocation(ssdoShader.ID, "noiseScale"), static_cast<float>(SCR_WIDTH) / 4.0f, static_cast<float>(SCR_HEIGHT) / 4.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gDepth);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, peelPosition);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, peelNormal);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, peelAlbedo);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, peelDepth);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, noiseTex);

        renderQuad();

        // Pass 6: edge-aware blur for SSDO detail stabilization.
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        blurShader.use();
        glUniform2f(glGetUniformLocation(blurShader.ID, "texelSize"), 1.0f / static_cast<float>(SCR_WIDTH), 1.0f / static_cast<float>(SCR_HEIGHT));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssdoColor);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gDepth);

        renderQuad();

        // Pass 7: direct light + coarse RSM GI + SSDO detail compensation.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.04f, 0.04f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        compositeShader.use();
        compositeShader.setVec3("lightPosWS", lightPosWorld);
        compositeShader.setVec3("viewPosWS", camera.position);
        compositeShader.setVec3("lightColor", lightBaseColor * pointLightIntensity);
        compositeShader.setMat4("invView", glm::inverse(view));
        compositeShader.setFloat("farPlane", SHADOW_FAR);
        compositeShader.setFloat("ambientStrength", ambientStrength);
        compositeShader.setFloat("rsmIntensity", rsmIntensity);
        compositeShader.setFloat("ssdoDetailStrength", ssdoDetailStrength);
        compositeShader.setInt("uEnableSSDODetail", enableSSDODetail ? 1 : 0);
        compositeShader.setFloat("diffuseStrength", 1.0f);
        compositeShader.setFloat("specularStrength", 0.05f);
        compositeShader.setFloat("shininess", 32.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gDepth);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, blurColor);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, rsmSmoothTex);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCube);

        renderQuad();

        // Optional debug marker for the point light.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        lightShader.use();
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);
        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPosWorld);
        lightModel = glm::scale(lightModel, glm::vec3(0.03f));
        lightShader.setMat4("model", lightModel);
        renderCube();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    roomModel.Terminate();
    bunnyModel.Terminate();
    armadilloModel.Terminate();

    glDeleteFramebuffers(1, &pointShadowFBO);
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteFramebuffers(1, &peelFBO);
    glDeleteFramebuffers(1, &rsmFBO);
    glDeleteFramebuffers(1, &rsmGiFBO);
    glDeleteFramebuffers(1, &rsmSmoothFBO);
    glDeleteFramebuffers(1, &ssdoFBO);
    glDeleteFramebuffers(1, &blurFBO);

    glDeleteTextures(1, &pointShadowCube);
    glDeleteTextures(1, &gPosition);
    glDeleteTextures(1, &gNormal);
    glDeleteTextures(1, &gAlbedo);
    glDeleteTextures(1, &gDepth);
    glDeleteTextures(1, &peelPosition);
    glDeleteTextures(1, &peelNormal);
    glDeleteTextures(1, &peelAlbedo);
    glDeleteTextures(1, &peelDepth);
    glDeleteTextures(1, &rsmFluxTex);
    glDeleteTextures(1, &rsmNormalTex);
    glDeleteTextures(1, &rsmDepthTex);
    glDeleteTextures(1, &rsmGiTex);
    glDeleteTextures(1, &rsmSmoothTex);
    glDeleteTextures(1, &ssdoColor);
    glDeleteTextures(1, &blurColor);
    glDeleteTextures(1, &noiseTex);

    glfwTerminate();
    return 0;
}
