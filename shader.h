#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();  // 析构函数生命
    // activate shader program
    void use();
    // uniform setting
    void setBool(const std::string &name, bool value) const; 
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4ById(int id, glm::mat4 matrix) const;
    void setMat4(const std::string &name, glm::mat4 matrix) const;
    void setMat3ById(int id, glm::mat3 matrix) const;
    void setMat3(const std::string &name, glm::mat3 matrix) const;
    void setVec3(const std::string &name, glm::vec3 _vec3) const;
};


#endif