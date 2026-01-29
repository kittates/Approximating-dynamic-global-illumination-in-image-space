#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../shader.h"

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture  // 纹理贴图
{
    unsigned int id;
    std::string type;
    aiString path;  // 用于避免重复加载
};

struct MaterialColors { // 纹理颜色
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    bool hasTexture;    // 标记是否有纹理贴图
};

class Mesh {
    public:
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;  // vertex index
        std::vector<Texture> textures;
        MaterialColors material;    // 材质颜色

        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, MaterialColors material = {glm::vec3(0.1f), glm::vec3(0), glm::vec3(0), 32.0f, false}) {
            this->vertices = vertices;
            this->indices = indices;
            this->textures = textures;
            this->material = material;
            setupMesh();    // 加载mesh数据，并初始化buffer
        }
        // 绘制当前mesh
        void Draw(Shader &shader) { 
            if(textures.size() > 0) {
                unsigned int diffuseNr = 1;
                unsigned int specularNr = 1;
                for(unsigned int i=0; i<textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    std::string number, name;
                    name = textures[i].type;
                    if(name == "texture_diffuse") number = std::to_string(diffuseNr++);
                    else if(name == "texture_specular") number = std::to_string(specularNr++);
                    shader.setInt("material." + name + number, i);  // e.g. material.texture_diffuse3
                    glBindTexture(GL_TEXTURE_2D, textures[i].id);
                }
                glActiveTexture(GL_TEXTURE0);
                shader.setBool("material.useTexture", true);
            }
            // TODO::既有贴图又有颜色该如何？
            else {
                shader.setBool("material.useTexture", false);
                shader.setVec3("material.ambientColor", material.ambient);
                shader.setVec3("material.diffuseColor", material.diffuse);
                shader.setVec3("material.specularColor", material.specular);
                shader.setFloat("material.shininess", material.shininess);
            }
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        void clearBuffer() {
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }

    private:
        unsigned int VAO, VBO, EBO;

        void setupMesh() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);   // 第三个需要的是"指向连续内存首地址的指针", 或&vertices[0]
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Normal)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, TexCoords)));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
        }


};
