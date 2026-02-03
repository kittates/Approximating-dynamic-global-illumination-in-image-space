#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SENSITIVITY = 0.5f;
const float SPEED = 2.5f;
const float FOV = 60.0f;

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    JUMP,
    JUMP_RELEASE,
};

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;   // up不仅是(0,1,0),而是camera的Y轴
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float fov;

    Camera(glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f), 
    float _yaw = YAW, float _pitch = PITCH):
    movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), fov(45.0f), front(glm::vec3(0.0f, -1.0f, 0.0f)) {
        position = _position;
        worldUp = _up;
        yaw = _yaw;
        pitch = _pitch;
        // update view
        updateViewDirection();
    }
    /**
     * @position: the position of the camera
     * @up: up vector
     * @target: the point where camera is looking at
     */
    glm::mat4 myLookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        glm::vec3 direction = target - position;
        glm::vec3 right = glm::cross(direction, up);
        glm::mat4 trans;
        trans = glm::mat4(1.0f, 0.0f, 0.0f,0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0,
                        -position.x, -position.y, -position.z, 1.0f);
        glm::mat4 rotate;
        rotate = glm::mat4(right.x, up.x, -direction.x, 0,
                        right.y, up.y, -direction.y, 0,
                        right.z, up.z, -direction.z, 0,
                        0.0f, 0.0f, 0.0f, 1.0f);
        return rotate * trans;
    }

    glm::mat4 getLookAt() {
        glm::mat4 view;
        // view = glm::lookAt(position, position + front, up); // camera真正的up，不是worldUp
        view = myLookAt(position, position + front, up);
        return view;
    }
    void mouseMovement(float xoffset, float yoffset, bool constrainView = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;
        pitch += yoffset;
        yaw += xoffset;
        
        if(constrainView) {
            pitch = pitch > 89.0f ? 89.0f : pitch;
            pitch = pitch < -89.0f ? -89.0f : pitch;
        }
        updateViewDirection();
    }
    void mouseScroll(float yoffset) {
        if(fov >= 1.0f && fov <= FOV) {
            fov -= yoffset;
        }
        fov = fov <= 1.0f ? 1.0f : fov;
        fov = fov >= FOV ? FOV : fov;
    }
    void keyboardMovement(Camera_Movement motion, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        if(motion == FORWARD) {
            position += front * velocity;
        }
        if(motion == BACKWARD) {
            position -= front * velocity;
        }
        if(motion == LEFT) {
            position -= right * velocity;
        }
        if(motion == RIGHT) {
            position += right * velocity;
        }
        if(motion == UP) {
            position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
        }
        if(motion ==DOWN) {
            position += glm::vec3(0.0f, -1.0f, 0.0f) * velocity;
        }
        // if(motion == JUMP) {
        //     if(position.y >= 0.25f) return;
        //     position += glm::vec3(0.0f, 0.3f, 0.0f) * velocity * 1.5f;
        // }
        // if(motion == JUMP_RELEASE) {
        //     if(position.y <= 0) return;
        //     position -= glm::vec3(0.0f, 0.3f, 0.0f) * velocity * 1.5f;
        // }
    }

private:
    void updateViewDirection() {
        glm::vec3 _front;
        _front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        _front.y = sin(glm::radians(pitch));
        // _front.y = 1.0;
        _front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
        front = glm::normalize(_front);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
        
};


#endif