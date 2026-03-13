#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "shader.h"
#include "camera.h"
#include "loadModel/model.h"

#include <iostream>
#include <random>
#include <map>
#include <chrono>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int loadTexture(const char *path);
void setSpotConfig(Shader &shader, glm::mat4 view, glm::vec3 spotLightPos);
// render all objs
void renderAllObjs(unsigned int floorTexture, unsigned int glassTexture, unsigned int cubeTexture, unsigned int cubeMapTexture, 
        unsigned int dynamicEnvSkyboxTexture, bool dynamicEnvSkyboxTexture_Enable,
        unsigned int planeVAO, unsigned int lightCubeVAO, unsigned int cubeVAO, unsigned int grassVAO, unsigned int skyboxVAO,
        Shader &shader, Shader &lightShader, Shader &ourShader, Shader &ourShader_rectify, Shader &edgeShader, 
        Shader &skyboxShader, Shader &cubeShader, Shader &dynamicCubeShader,
        glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection,
        const std::vector<glm::vec3> &vegetation, Model &ourModel);
std::vector<glm::vec3> genRandom(int num);
// VAO_VBO_data
void prerequisite_data(unsigned int &cubeVAO, unsigned int &cubeVBO, unsigned int &lightCubeVAO, unsigned int &grassVAO,
            unsigned int &planeVAO, unsigned int &planeVBO, unsigned int &screenVAO, unsigned int &screenVBO,
            unsigned int &skyboxVAO, unsigned int &skyboxVBO,
            const float cubeVertices[], size_t cubeSize,
            const float planeVertices[], size_t planeSize,
            const float quadVertices[], size_t quadSize,
            const float skyboxVertices[], size_t skyboxSize);
unsigned int loadCubeTexture(std::vector<std::string> faces);

// settings
// const unsigned int SCR_WIDTH = 1920;
// const unsigned int SCR_HEIGHT = 1080;
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 400;
const unsigned int ENV_SIZE = 512; // 正方形分辨率 用于dynamic skybox的framebuffer
const glm::vec3 dynamicCubeCoordinate = glm::vec3(2.0f, 1.5f, 1.2f);

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH  / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float near = 2.3f;
float far = 100.0f;

float light_step = 0.0f;
float light_height = 0.0f;
float ratio = 15.0f;
float outLine = 0.005f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetKeyCallback(window, keyboard_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // always pass the depth test (same effect as glDisable(GL_DEPTH_TEST))
    // glDepthRange(1.0, 0);
    // glClearDepth(0.0f);
    
    glEnable(GL_STENCIL_TEST);  // 模板测试

    glEnable(GL_BLEND); // 开启混合模式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE); 
    glCullFace(GL_BACK);    // 剔除背向面
    glFrontFace(GL_CCW);    // CCW为正向面
    // build and compile shaders
    // -------------------------

    Shader lightShader("./shader/light/light.vert", "./shader/light/light.frag");
    // the two shaders below are about cube
    // shader -> floor
    Shader shader("./shader/depth_testing/depth_testing.vert", "./shader/depth_testing/depth_testing.frag");
    // cubeShader是两个cube的shader
    Shader cubeShader("./shader/depth_testing/cube.vert", "./shader/depth_testing/cube.frag");
    Shader edgeShader("./shader/depth_testing/shaderSingleColor_rectify.vert", "./shader/depth_testing/shaderSingleColor.frag");
    // the two shaders below are about model 
    Shader ourShader("./shader/backpack/backpack.vert", "./shader/backpack/backpack.frag");
    Shader ourShader_rectify("./shader/depth_testing/shaderSingleColor_rectify.vert", "./shader/depth_testing/shaderSingleColor.frag");
    
    Shader screenShader("./shader/screenBuffer/screenBuffer.vert", "./shader/screenBuffer/screenBuffer.frag");
    Shader skyboxShader("./shader/skybox/skybox.vert", "./shader/skybox/skybox.frag");
    // 下面这个cube是我要将dynamic skybox实施的box
    Shader dynamicCubeShader("./shader/skybox/dynamicSkybox.vert", "./shader/skybox/dynamicSkybox.frag");

    Model ourModel("../models/roadBike/roadBike.obj");
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float cubeVertices[] = {    // // 逆时针环绕顺序
    // positions          // normals           // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
    float planeVertices[] = {   // 逆时针环绕顺序
        // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
         -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f,								
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
    };
    /*float quadVertices[] = {     // top
        // positions   // texCoords
        -0.3f, 1.0f,  0.0f, 1.0f,
        -0.3f, 0.7f,  0.0f, 0.0f,
         0.3f, 0.7f,  1.0f, 0.0f,

        -0.3f, 1.0f,  0.0f, 1.0f,
         0.3f, 0.7f,  1.0f, 0.0f,
         0.3f, 1.0f,  1.0f, 1.0f
    };*/
    float quadVertices[] = {    // left bottom
        -1.0f, -0.5f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        -0.5f, -1.0f,  1.0f, 0.0f,
        
        -1.0f, -0.5f,  0.0f, 1.0f,
        -0.5f,  -1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  1.0f, 1.0f
    };
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };
    std::vector<std::pair<glm::vec3, glm::vec3>> envCubeLookAt = {
        {glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)},
        {glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)},
        {glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)},
        {glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)},
        {glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)},
        {glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)},
    };


    // skycube
    std::string baseDir = "./images/skybox/";
    std::vector<std::string> faces {
        baseDir + "right.jpg",
        baseDir + "left.jpg",
        baseDir + "top.jpg",
        baseDir + "bottom.jpg",
        baseDir + "front.jpg",
        baseDir + "back.jpg"
    };
    std::vector<glm::vec3> vegetation = genRandom(10);  // grass position

    unsigned int cubeVAO, cubeVBO, lightCubeVAO, grassVAO;
    unsigned int planeVAO, planeVBO;
    unsigned int screenVAO, screenVBO;
    unsigned int skyboxVBO, skyboxVAO;
    
    prerequisite_data(cubeVAO, cubeVBO, lightCubeVAO, grassVAO, planeVAO, planeVBO, screenVAO, screenVBO, skyboxVAO, skyboxVBO,
                cubeVertices, sizeof(cubeVertices), planeVertices, sizeof(planeVertices), quadVertices, sizeof(quadVertices), skyboxVertices, sizeof(skyboxVertices));

    // load textures
    // -------------
    unsigned int cubeTexture  = loadTexture("./images/marble.jpg");
    unsigned int floorTexture = loadTexture("./images/metal.png");
    // unsigned int grassTexture = loadTexture("./images/grass.png");
    unsigned int glassTexture = loadTexture("./images/blending_transparent_window.png");
    unsigned int cubeMapTexture = loadCubeTexture(faces);
    // shader configuration
    // --------------------
    // 不同shader中的uniform sampler2D可以设置相同的编号，因为这两个shader不会同时运行，每次只会激活一个shader
    shader.use();
    shader.setInt("texture1", 0);   // 设置cube shader编号
    cubeShader.use();
    cubeShader.setInt("texture1", 0);
    screenShader.use();
    screenShader.setInt("screenTexture", 0);
    dynamicCubeShader.use();
    dynamicCubeShader.setInt("skybox", 0);
    ourShader.use();
    ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    // self framebuffer
    // -----------
    unsigned int framebuffer, envFramebuffer;
    glGenFramebuffers(1, &framebuffer);
    glGenFramebuffers(1, &envFramebuffer);
    
    // 2D
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    unsigned int texColorBuffer;    // 得到的结果存在这张纹理中
    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0); // framebuffer的颜色附件绑定在texColorBuffer的面上
    // renderbuffer obj-> depth & stencil   
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    // glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);   // depth stencil attachment
    // check framebuffer status
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: TEXTURE_2D Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);   // 解绑，切回到默认的framebuffer，即屏幕
    
    // 3D
    glBindFramebuffer(GL_FRAMEBUFFER, envFramebuffer);
    unsigned int dynamicEnvSkyboxTexture;
    glGenTextures(1, &dynamicEnvSkyboxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicEnvSkyboxTexture);
    for(GLenum i=0; i<6; i++) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
            0, GL_RGB, ENV_SIZE, ENV_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL    
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    // depth and stencil
    unsigned int envRbo;
    glGenRenderbuffers(1, &envRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, envRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ENV_SIZE, ENV_SIZE);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        envRbo
    );



    // render loop
    // -----------
    while(!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        
        // bind self framebuffer
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        
        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClearStencil(0);
        
        // 绘制dynamic skybox得到纹理
        // 必须要更改视口，否则得到的纹理图片和立方体面不匹配，造成渲染结果扭曲
        glViewport(0, 0, ENV_SIZE, ENV_SIZE);
        glm::mat4 envCubePorjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, envFramebuffer);
        for(unsigned int i=0; i<6; i++) {  
            glFramebufferTexture2D(     // 将framebuffer的COLOR_ATTACHMENT0挂在到当前面
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                dynamicEnvSkyboxTexture,
                0
            );
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "ERROR::FRAMEBUFFER::TEXTURE_3D Framebuffer is not complete!" << std::endl;
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, dynamicCubeCoordinate);
            glm::mat4 view = glm::lookAt(dynamicCubeCoordinate, dynamicCubeCoordinate + envCubeLookAt[i].first, envCubeLookAt[i].second);
            // 这里最开始并没有设置projection，所以会导致bug，所以直接在renderAllObjs中设置projection了
            renderAllObjs(floorTexture, glassTexture, cubeTexture, cubeMapTexture, dynamicEnvSkyboxTexture, false,
                planeVAO, lightCubeVAO, cubeVAO, grassVAO, skyboxVAO,
                shader, lightShader, ourShader, ourShader_rectify, edgeShader, skyboxShader, cubeShader, dynamicCubeShader,
                model, view, envCubePorjection,
                vegetation, ourModel
            );
        }
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // 必须要改回视口
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        // 绘制后视镜

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // glUniform1f(glGetUniformLocation(shader.ID, "near"), near);
        // glUniform1f(glGetUniformLocation(shader.ID, "far"), far);
        glm::mat4 model = glm::mat4(1.0f);
        camera.yaw += 180.0;           // reverse view direction
        camera.mouseMovement(0, 0, false);
        glm::mat4 view = camera.getLookAt();
        camera.yaw -= 180.0;        // recover view direction
        camera.mouseMovement(0, 0, true);
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // TODO::应该把所有的有关projection的设置放置到while外
        shader.use();
        shader.setMat4("projection", projection);

        cubeShader.use();
        cubeShader.setMat4("projection", projection);
        cubeShader.setVec3("cameraPos", camera.position);
        dynamicCubeShader.use();
        dynamicCubeShader.setMat4("projection", projection);
        dynamicCubeShader.setVec3("cameraPos", camera.position);
        
        // render all objs firstly
        renderAllObjs(floorTexture, glassTexture, cubeTexture, cubeMapTexture, dynamicEnvSkyboxTexture, true,
            planeVAO, lightCubeVAO, cubeVAO, grassVAO, skyboxVAO,
            shader, lightShader, ourShader, ourShader_rectify, edgeShader, skyboxShader, cubeShader, dynamicCubeShader,
            model, view, projection,
            vegetation, ourModel
        );
        
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        view = camera.getLookAt();
        // shader.use();
        // shader.setMat4("view", view);
        // cubeShader.use();
        // cubeShader.setMat4("view", view);
    
        // render all objs again
        renderAllObjs(floorTexture, glassTexture, cubeTexture, cubeMapTexture, dynamicEnvSkyboxTexture, true,
            planeVAO, lightCubeVAO, cubeVAO, grassVAO, skyboxVAO,
            shader, lightShader, ourShader, ourShader_rectify, edgeShader, skyboxShader, cubeShader, dynamicCubeShader,
            model, view, projection,
            vegetation, ourModel
        );

        // render self framebuffer
        glDisable(GL_DEPTH_TEST);   // 避免self framebuffer因深度被discard
        screenShader.use();
        glBindVertexArray(screenVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColorBuffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &screenVAO);
    
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &screenVBO);

    glDeleteFramebuffers(1, &framebuffer);
    glDeleteFramebuffers(1, &envFramebuffer);

    glDeleteRenderbuffers(1, &rbo);
    glDeleteRenderbuffers(1, &envRbo);

    glDeleteTextures(1, &cubeTexture);
    glDeleteTextures(1, &floorTexture);
    glDeleteTextures(1, &glassTexture);
    glDeleteTextures(1, &cubeMapTexture);
    glDeleteTextures(1, &texColorBuffer);
    glDeleteTextures(1, &dynamicEnvSkyboxTexture);

    glDeleteProgram(lightShader.ID);
    glDeleteProgram(shader.ID);
    glDeleteProgram(cubeShader.ID);
    glDeleteProgram(edgeShader.ID);
    glDeleteProgram(ourShader.ID);
    glDeleteProgram(ourShader_rectify.ID);
    glDeleteProgram(screenShader.ID);
    glDeleteProgram(skyboxShader.ID);

    ourModel.Terminate();
    glfwTerminate();
    return 0;
}

// VAO_VBO_data
// unsigned int 在内部被修改，故需要引用传递
void prerequisite_data(unsigned int &cubeVAO, unsigned int &cubeVBO, unsigned int &lightCubeVAO, unsigned int &grassVAO,
            unsigned int &planeVAO, unsigned int &planeVBO, unsigned int &screenVAO, unsigned int &screenVBO,
            unsigned int &skyboxVAO, unsigned int &skyboxVBO,
            const float cubeVertices[], size_t cubeSize,
            const float planeVertices[], size_t planeSize,
            const float quadVertices[], size_t quadSize,
            const float skyboxVertices[], size_t skyboxSize) {
    glGenVertexArrays(1, &cubeVAO);
    glGenVertexArrays(1, &lightCubeVAO);
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &cubeVBO);
    // upload
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    // glBufferData将数据上传到了GPU内存中的VBO对象本身，数据时持久存储在VBO的，不会因为解绑而消失
    glBufferData(GL_ARRAY_BUFFER, cubeSize, cubeVertices, GL_STATIC_DRAW);
    // cube
    glBindVertexArray(cubeVAO);
    // glBindBuffer(GL_ARRAY_BUFFER, cubeVBO); // 多余，应为上一步cubeVBO仍旧绑定在GL_ARRAY_BUFFER
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
    // light cube
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    // grass cube
    glBindVertexArray(grassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    // plane VAO
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, planeSize, planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);  

    // screen quad VAO
    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);
    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER,  quadSize, quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // skybox
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    //skyboxSize是传入的数据字节数，不是元素个数
    glBufferData(GL_ARRAY_BUFFER, skyboxSize, skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

// obj render
// unsigned int 内部不修改，故不需要引用
void renderAllObjs(unsigned int floorTexture, unsigned int glassTexture, unsigned int cubeTexture, unsigned int cubeMapTexture, 
                unsigned int dynamicEnvSkyboxTexture, bool dynamicEnvSkyboxTexture_Enable,
                unsigned int planeVAO, unsigned int lightCubeVAO, unsigned int cubeVAO, unsigned int grassVAO, unsigned int skyboxVAO,
                Shader &shader, Shader &lightShader, Shader &ourShader, Shader &ourShader_rectify, Shader &edgeShader, 
                Shader &skyboxShader, Shader &cubeShader, Shader &dynamicCubeShader,
                glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection,
                const std::vector<glm::vec3> &vegetation, Model &ourModel) {
        
        // floor
        shader.use();
        glDisable(GL_CULL_FACE);
        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        shader.setMat4("projection", projection);
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setMat4("view", view);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0); 
        glEnable(GL_CULL_FACE); 

        // light
        float r = 2.0f; 
        float speed = 0.3f;
        light_step = glfwGetTime() * 2.5f;
        float _x = r * cos(light_step * speed);
        float _z = r * sin(light_step * speed);
        glm::vec3 spotLightPos = glm::vec3(_x, light_height, _z);
        glm::mat4 light_model = glm::mat4(1.0f);
        light_model = glm::translate(light_model, spotLightPos);
        light_model = glm::scale(light_model, glm::vec3(0.2f));
        lightShader.use();
        lightShader.setMat4("model", light_model);
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);
        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // bicycle
        glStencilMask(0xff);
        glStencilFunc(GL_ALWAYS, 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0, -0.3f, 0));
        model = glm::scale(model, glm::vec3(1.5f)); // 使用scale而非normal scale
        ourShader.use();
        ourShader.setMat4("model", model);  // model
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        setSpotConfig(ourShader, view, spotLightPos);   // config spot
        ourModel.Draw(ourShader);
        // scaled bicycle
        glStencilMask(0x00);
        glStencilFunc(GL_NOTEQUAL, 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0, -0.3f, 0));
        model = glm::scale(model, glm::vec3(1.5f));
        ourShader_rectify.use();
        ourShader_rectify.setFloat("uOutline",outLine);
        ourShader_rectify.setMat4("model", model);
        ourShader_rectify.setMat4("view", view);
        ourShader_rectify.setMat4("projection", projection);
        setSpotConfig(ourShader_rectify, view, spotLightPos);   // config spot
        ourModel.Draw(ourShader_rectify);
        glStencilMask(0xff);
        glClear(GL_STENCIL_BUFFER_BIT);

        // cubes-1    normal box    
        glStencilMask(0xff);    // 所有位可写入
        glStencilFunc(GL_ALWAYS, 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        cubeShader.use();
        cubeShader.setMat4("view", view);
        cubeShader.setMat4("projection", projection);
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        // glBindTexture(GL_TEXTURE_)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0f, 0.05f, -1.0f));
        cubeShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // scaled cube-1
        glStencilMask(0x00);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        edgeShader.use();
        edgeShader.setFloat("uOutline", outLine);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0f, 0.05f, -1.0f));   // glm右乘:T R S
        // model = glm::scale(model, glm::vec3(1.05f));
        edgeShader.setMat4("model", model);
        edgeShader.setMat4("view", view);
        edgeShader.setMat4("projection", projection);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glStencilMask(0xff);    // 否则无法clear stencil buffer
        glClear(GL_STENCIL_BUFFER_BIT);
        // cube-2
        // glStencilMask(0xff);    // 所有位可写入
        // glStencilFunc(GL_ALWAYS, 1, 0xff);
        // glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        // // shader.use();
        // cubeShader.use();
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(2.0f, 0.05f, 0.0f));
        // // shader.setMat4("model", model);
        // cubeShader.setMat4("model", model);
        // glDrawArrays(GL_TRIANGLES, 0, 36);
        // // scaled cube-2
        // glStencilMask(0x00);
        // glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        // glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // edgeShader.use();
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(2.0f, 0.05f, 0.0f));
        // // model = glm::scale(model, glm::vec3(1.05f));
        // edgeShader.setMat4("model", model);
        // edgeShader.setMat4("view", view);
        // edgeShader.setMat4("projection", projection);
        // glDrawArrays(GL_TRIANGLES, 0, 36);
        // glBindVertexArray(0);
        // glStencilMask(0xff);
        // glClear(GL_STENCIL_BUFFER_BIT); 

        // skybox   最用渲染skybox，利用Early-Z优化性能
        // 先渲染天空盒，在渲染grass，可以是glass看到天空盒
        // glDepthMask(GL_FALSE);  // skybox禁止深度写入    // 使用Early-Z优化后不必要了
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL); // 天空盒设置的NDC为
        skyboxShader.use();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));   // 去除view的位移，仅保留旋转
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // glDepthMask(GL_TRUE);   // recover
        glDepthFunc(GL_LESS);   // recover
        
        // dynamic skybox 
        // normal box
        if(dynamicEnvSkyboxTexture_Enable) {
            glDisable(GL_STENCIL_TEST);
            dynamicCubeShader.use();
            dynamicCubeShader.setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model, dynamicCubeCoordinate);
            model = glm::scale(model, glm::vec3(2.5f));
            dynamicCubeShader.setMat4("model", model);
            glBindVertexArray(cubeVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicEnvSkyboxTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glEnable(GL_STENCIL_TEST);
        }


        // grass
        // glBindVertexArray(grassVAO);
        // glActiveTexture(GL_TEXTURE0);
        // // glBindTexture(GL_TEXTURE_2D, grassTexture);
        // glBindTexture(GL_TEXTURE_2D, glassTexture);
        // std::map<float, glm::vec3> sorted;
        // // from far to near in camera direction
        // for(unsigned int i=0; i<vegetation.size(); i++) {
        //     float distance = glm::length(camera.position - vegetation[i]);
        //     sorted[distance] = vegetation[i];   // sorted map
        // }
        // shader.use();
        // // for(unsigned int i=0; i<vegetation.size(); i++) {
        // for(std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); it++) {
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, it->second);
        //     shader.setMat4("model", model);
        //     glDrawArrays(GL_TRIANGLES, 0, 6);   // 只绘制一个面
        // }

        

        

}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.keyboardMovement(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.keyboardMovement(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.keyboardMovement(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.keyboardMovement(RIGHT, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) 
        camera.keyboardMovement(JUMP, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
        camera.keyboardMovement(JUMP_RELEASE, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS)
        camera.keyboardMovement(UP, deltaTime);   
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS)
        camera.keyboardMovement(DOWN, deltaTime);   
    if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if(near <= 0.1) return;
        near -= 0.2;
    }
    if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if(near >= far) return;
        near += 0.2;
    }
    if(glfwGetKey(window, GLFW_KEY_EQUAL)==GLFW_PRESS)
        light_step += 0.1;
    if(glfwGetKey(window, GLFW_KEY_MINUS)==GLFW_PRESS)
        light_step -= 0.1;
    if(glfwGetKey(window, GLFW_KEY_N)==GLFW_PRESS) {
        if(light_height <= -50.0f) return;
        light_height -= 0.01f;
    }
    if(glfwGetKey(window, GLFW_KEY_M)==GLFW_PRESS) {
        if(light_height >= 50.0f) return;
        light_height += 0.01f;
    }
    if(glfwGetKey(window, GLFW_KEY_K)==GLFW_PRESS)
        ratio = ratio >= 85.0 ? 85.0 : ratio + 0.5;
    if(glfwGetKey(window, GLFW_KEY_L)==GLFW_PRESS)
        ratio = ratio <= 3.0 ? 3.0 : ratio - 0.5;
    if(glfwGetKey(window, GLFW_KEY_PAGE_UP)==GLFW_PRESS)
        outLine += 0.001f;
    if(glfwGetKey(window, GLFW_KEY_PAGE_DOWN)==GLFW_PRESS)
        outLine -= 0.001f;

}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        std::cout << "space press" << std::endl;
    }
    if(key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        std::cout << "space release" << std::endl;

    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.mouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.mouseMovement(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path)
{
    stbi_set_flip_vertically_on_load(true);
    
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if(format == GL_RGBA) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    stbi_set_flip_vertically_on_load(false);    // recover 
    return textureID;
}

// load cubeTexture
unsigned int loadCubeTexture(std::vector<std::string> faces) {
    // stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    
    for(unsigned int i=0; i<faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if(data) {
            GLenum format;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;
            // gen cubeTexture
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data    
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void setSpotConfig(Shader &shader, glm::mat4 view, glm::vec3 spotLightPos) {
    shader.setVec3("spotLight.position", glm::vec3(view * glm::vec4(spotLightPos, 1.0f)));
    shader.setVec3("spotLight.direction", glm::vec3(view * (glm::vec4(spotLightPos, 1.0f) - glm::vec4(0, 0, 0, 1.0f))));
    shader.setVec3("spotLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
    shader.setVec3("spotLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));    // 0.5
    shader.setVec3("spotLight.specular", glm::vec3(1.4f, 1.4f, 1.4f));   // 1.0
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09f);
    shader.setFloat("spotLight.quadratic", 0.032f);
    shader.setFloat("spotLight.cutOff_phi", glm::cos(glm::radians(ratio)));
    shader.setFloat("spotLight.cutOff_gamma", glm::cos(glm::radians(ratio + 3.0)));
}

// gen grass position
std::vector<glm::vec3> genRandom(int num) {
    // use current time (ms) as seed
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    uint32_t seed = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count() & 0xffffffff);
    std::mt19937 gen(seed);

    std::uniform_real_distribution<float> dis_x(-4.0f, 4.0f);
    std::uniform_real_distribution<float> dis_z(-4.0f, 4.0f);

    std::vector<glm::vec3> vegetation;
    vegetation.reserve(num);
    for(unsigned int i = 0; i < num; ++i) {
        float x = dis_x(gen);
        float z = dis_z(gen);
        vegetation.emplace_back(x, 0.0f, z);
    }
    return vegetation;
}

/*std::vector<glm::vec3> genRandom(int num) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_x(-4.0f, 4.0f);
    std::uniform_int_distribution<> dis_z(-4.0f, 4.0f);

    std::vector<glm::vec3> vegetation;
    for(unsigned int i=0; i<num; i++) {
        float x = dis_x(gen);
        float z = dis_z(gen);
        vegetation.push_back(glm::vec3(x, 0.0f, z));
    }
    return vegetation;
}*/