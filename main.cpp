#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include "shader.h" 
#include "./loadModel/model.h"
#include "camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, 
                    float &opacity, 
                    float &ratio, 
                    float &deltaTime,
                    Camera &camera,
                    float &light_height,
                    float &light_step);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(char const *path);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float light_step = 0;

// cursor init
float lastX = 400;
float lastY = 300;

bool firstMouse = true;
float fov = 45.0f;

// Camera camera(glm::vec3(0.0f, 0.0, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
Camera camera(glm::vec3(3.7342, 2.0652f, -1.6181f), glm::vec3(0.0f, 1.0f, 0.0f), 144.3973f, -26.8f);

int main() {
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 16);   // 设置多采样
    
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    SCR_WIDTH = mode->width / 1.5f;
    SCR_HEIGHT = mode->height / 1.5f;

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "I'm sucker", NULL, NULL);
    if(window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window,GLFW_CURSOR, GLFW_CURSOR_HIDDEN);   // tell glfw to hide cursor
    glfwSetScrollCallback(window, scroll_callback);

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    glfwSetCursorPos(window, w * 0.5, h * 0.5);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // glClearColor(0.87, 0.72, 0.53, 0.0f);   // desert color

    float cube[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    unsigned int VAO, lightVAO, VBO, EBO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glGenVertexArrays(1, &lightVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    // cube
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);   // vertex
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // TexCoords
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // light cube, same as cube
    glBindVertexArray(lightVAO) ;
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    Shader lightShader("./shader/light/light.vert", "./shader/light/light.frag");
    Shader ourShader("./shader/backpack/backpack.vert", "./shader/backpack/backpack.frag");


    float opacity=0.0;
    // float ratio = (float)SCR_WIDTH / SCR_HEIGHT;
    float ratio = 15.0f;
    float light_height = 0.0f;

    ourShader.use();

    // 设置光的base强度
    glUniform3f(glGetUniformLocation(ourShader.ID, "lightColor"), 1.0f, 1.0f, 1.0f);
    
    /* 设置Fragment材质
    // shader.setVec3("material.ambient", glm::vec3(0.0f, 0.1f, 0.06f));
    // shader.setVec3("material.diffuse", glm::vec3(0.0f, 0.50980392f, 0.50980392f));
    // shader.setVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
    // shader.setFloat("material.shininess", 64.0f);
    // 设置光照的属性。调整不同的值来设置不同的光照强度
    // glm::vec3 lightColor = glm::vec3(1.0f);
    // glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
    // glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
    // shader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    // shader.setVec3("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    // shader.setVec3("light.specular", glm::vec3(2.0f, 2.0f, 2.0f));
    // shader.setFloat("light.constant", 1.0);
    // shader.setFloat("light.linear", 0.014);
     shader.setFloat("light.quadratic", 0.0007);*/

    unsigned int our_model = glGetUniformLocation(ourShader.ID, "model");
    unsigned int our_view = glGetUniformLocation(ourShader.ID, "view");
    unsigned int our_projection = glGetUniformLocation(ourShader.ID, "projection");
    unsigned int modelLightLoc = glGetUniformLocation(lightShader.ID, "model");
    unsigned int viewLightLoc = glGetUniformLocation(lightShader.ID, "view");
    unsigned int projectionLightLoc = glGetUniformLocation(lightShader.ID, "projection");
    unsigned int normalMatrix = glGetUniformLocation(ourShader.ID, "normalMatrix");
    
    std::string path_backpack = "./models/backpack/backpack.obj";
    std::string path_CornellBox = "./models/cornell-box/CornellBox-Original.obj";
    
    Model ourModel((char*)(path_CornellBox.c_str()));
    while(!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, opacity, ratio, deltaTime,camera, light_height, light_step);   // 手动封装
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // cube
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.8));
        model = glm::translate(model, glm::vec3(0, -0.6, 0));
        // cube_model = glm::rotate(cube_model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = camera.getLookAt();
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(camera.fov), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        // glm::mat3 NormalMatrix = glm::transpose(glm::inverse(glm::mat3(cube_model)));
        // glm::mat3 NormalMatrix = glm::inverse(glm::transpose(glm::mat3(view * model)));
        glm::mat3 NormalMatrix = glm::transpose(glm::inverse(glm::mat3(view * model)));

        
        ourShader.use();
        ourShader.setMat4ById(our_model, model);
        ourShader.setMat4ById(our_view, view);
        ourShader.setMat4ById(our_projection, projection);
        ourShader.setMat3ById(normalMatrix, NormalMatrix);
        ourShader.setFloat("material.shininess", 64.0f);
        ourModel.Draw(ourShader);


        // light
        float r = 4.0f; 
        float speed = 0.3f;
        float _x = r * cos(light_step * speed);
        float _z = r * sin(light_step * speed);
        // float _x = r * cos(45);
        // float _z = r * sin(45);
        // float t = glfwGetTime();
        // float omega = 1.0f;
        // float nu = 0.7f;
        // float A = 0.6f;
        // float R = 3.0f;
        // float theta = omega * t;
        // float phi = glm::half_pi<float>() + A * sin(nu * t);
        // // float phi = omega * t;
        // float _x = r * sin(phi) * sin(theta);
        // float _y = r * sin(phi) * cos(theta);
        // float _z = r * cos(phi);

        // multi_light
        for(int i=0; i<4; i++) {
            // 设置光源的参数
            glm::mat4 light_model = glm::mat4(1.0f);
            // lightPos = glm::vec3(_x, light_height, _z); // 光照旋转
            // shader.setVec3("lightPos", glm::vec3(glm::vec4(lightPos, 1.0f)));
            // shader.setVec3("lightPos", glm::vec3(glm::vec4(pointLightPositions[i], 1.0f)));
            
            light_model = glm::translate(light_model, pointLightPositions[i]);
            light_model = glm::scale(light_model, glm::vec3(0.2f));

            // point light
            ourShader.use();
            ourShader.setVec3("pointLights[" + std::to_string(i) + "].position", glm::vec3(view * glm::vec4(pointLightPositions[i], 1.0f)));
            ourShader.setFloat("pointLights[" + std::to_string(i) + "].constant", 1.0f);
            ourShader.setFloat("pointLights[" + std::to_string(i) + "].linear", 0.09f);
            ourShader.setFloat("pointLights[" + std::to_string(i) + "].quadratic", 0.032f);
            ourShader.setVec3("pointLights[" + std::to_string(i) + "].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
            ourShader.setVec3("pointLights[" + std::to_string(i) + "].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            ourShader.setVec3("pointLights[" + std::to_string(i) + "].specular", glm::vec3(1.0f, 1.0f, 1.0f));
            glm::vec3 direction = glm::normalize(glm::vec3(view * (glm::vec4(pointLightPositions[i], 1.0) - glm::vec4(0, 0, 0, 1))));
            ourShader.setVec3("pointLights[" + std::to_string(i) + "].direction", direction);
            // shader.setFloat("pointLights[" + std::to_string(i) + "].cutOff_phi", glm::cos(glm::radians(ratio)));
            // shader.setFloat("pointLights[" + std::to_string(i) + "].cutOff_gamma", glm::cos(glm::radians(ratio + 5.0)));

            lightShader.use();
            lightShader.setMat4ById(modelLightLoc, light_model);
            lightShader.setMat4ById(viewLightLoc, view);
            lightShader.setMat4ById(projectionLightLoc, projection);

            glBindVertexArray(lightVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        /* parallel light
        // shader.use();
        // shader.setVec3("dirLight.direction", glm::vec3(view * glm::vec4(-0.2f, -1.0f, -0.3f, 1.0f)));
        // shader.setVec3("dirLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        // shader.setVec3("dirLight.diffuse", glm::vec3(0.6f, 0.6f, 0.6f));
        shader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f)); */
        
        // spot light
        // glm::vec3 spotLightPos = glm::vec3(2.5f, light_height, 0.0f);
        glm::vec3 spotLightPos = glm::vec3(_x, light_height, _z);
        glm::mat4 light_model = glm::mat4(1.0f); 
        light_model = glm::translate(light_model, spotLightPos);
        light_model = glm::scale(light_model, glm::vec3(0.2f));
        lightShader.use();
        lightShader.setMat4ById(modelLightLoc, light_model);
        lightShader.setMat4ById(viewLightLoc, view);
        lightShader.setMat4ById(projectionLightLoc, projection);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        ourShader.use();
        ourShader.setVec3("spotLight.position", glm::vec3(view * glm::vec4(spotLightPos, 1.0f)));
        ourShader.setVec3("spotLight.direction", glm::vec3(view * (glm::vec4(spotLightPos, 1.0f) - glm::vec4(0, 0, 0, 1.0f))));

        ourShader.setVec3("spotLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        ourShader.setVec3("spotLight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        ourShader.setVec3("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);

        ourShader.setFloat("spotLight.cutOff_phi", glm::cos(glm::radians(ratio)));
        ourShader.setFloat("spotLight.cutOff_gamma", glm::cos(glm::radians(ratio + 3.0)));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    ourModel.Terminate();
    glDeleteShader(ourShader.ID);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteProgram(ourShader.ID);
    glDeleteProgram(lightShader.ID);
    
    glfwTerminate();


    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0,0,width,height);
}

void processInput(GLFWwindow* window, 
                    float &opacity, 
                    float &ratio, 
                    float &deltaTime,
                    Camera &camera,
                    float &light_height,
                    float &light_step) {
    if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS)
        glfwSetWindowShouldClose(window,true);
    if(glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS) {
        opacity = opacity >= 1.0 ? 1.0 : opacity + 0.01;
    }
    if(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS) {
        opacity = opacity <=0.0 ? 0.0 : opacity - 0.01;
    }
    if(glfwGetKey(window, GLFW_KEY_K)==GLFW_PRESS) {
        ratio = ratio >= 85.0 ? 85.0 : ratio + 0.5;
    }
    if(glfwGetKey(window, GLFW_KEY_L)==GLFW_PRESS) {
        ratio = ratio <= 3.0 ? 3.0 : ratio - 0.5;
    }
    if(glfwGetKey(window, GLFW_KEY_N)==GLFW_PRESS) {
        if(light_height <= -50.0f) return;
        light_height -= 0.01f;
    }
    if(glfwGetKey(window, GLFW_KEY_M)==GLFW_PRESS) {
        if(light_height >= 50.0f) return;
        light_height += 0.01f;
    }
    // cameraPos
    if(glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS) {
        camera.keyboardMovement(FORWARD, deltaTime);
    }
    if(glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS) {
        camera.keyboardMovement(BACKWARD, deltaTime);
    }
    if(glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS) {
        camera.keyboardMovement(LEFT, deltaTime);
    }
    if(glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS) {
        camera.keyboardMovement(RIGHT, deltaTime);
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) {
        camera.keyboardMovement(UP, deltaTime);
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) {
        camera.keyboardMovement(DOWN, deltaTime);
    }
    if(glfwGetKey(window, GLFW_KEY_EQUAL)==GLFW_PRESS) {
        light_step += 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_MINUS)==GLFW_PRESS) {
        light_step -= 0.1;
    }
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if(firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.mouseMovement(xoffset, yoffset, true);
}
/**
 * @param xoffset: 水平滚动距离,一般鼠标为0
 * @param yoffset: 垂直滚动距离
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{   
    camera.mouseScroll(yoffset);
}

unsigned int loadTexture(char const *path) {
    stbi_set_flip_vertically_on_load(true);
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if(data) {
        // std::cout << "load successfully" << std::endl;
        GLenum format;
        if(nrChannels == 1) format = GL_RED;
        else if(nrChannels == 3) format = GL_RGB;
        else if(nrChannels == 4) format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        stbi_image_free(data);
        std::cout << "load texture failed" << std::endl;
    }
    return textureID;
}
