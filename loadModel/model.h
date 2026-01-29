#include <string>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../shader.h"
#include "mesh.h"
#include "../stb_image.h"

class Model {
    public:
        Model(char *path) {
            loadModel(path);
        }
        // ~Model() {
        //     for(unsigned int i=0; i<meshes.size(); i++) {
        //         meshes[i].clearBuffer();
        //     }
        // }

        void Draw(Shader shader) {
            for(unsigned int i=0; i<meshes.size(); i++) {
                meshes[i].Draw(shader);
            }
        }
        void Terminate() {
            for(unsigned int i=0; i<meshes.size(); i++) {
                meshes[i].clearBuffer();
            }
        }

    private:
        std::vector<Texture> textures_loaded;
        std::vector<Mesh> meshes;
        std::string directory;  // 存储文件路径目录，加载纹理会使用到

        void loadModel(std::string path) {
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(path,
                    aiProcess_Triangulate |
                    aiProcess_FlipUVs |
                    aiProcess_GenNormals);  // aiProcess_GenNormals Assimp自动计算法线
            if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
                return;
            }
            directory = path.substr(0, path.find_last_of('/'));
            processNode(scene->mRootNode, scene);
        }

        void processNode(aiNode *node, const aiScene *scene) {  // recursion
            // std::cout << "num of mesh: " << node->mNumMeshes << std::endl;
            for(unsigned int i=0; i<node->mNumMeshes; i++) {
                aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];    // node->mMeshes[i]中是索引,真实数据存放在scene->mMeshes中
                meshes.push_back(processMesh(mesh,scene));
            }
            // children
            for(unsigned int i=0; i<node->mNumChildren; i++) {
                processNode(node->mChildren[i], scene);
            }
        }
        Mesh processMesh(aiMesh *mesh, const aiScene *scene) {      // aiMesh->Mesh
            // 处理的数据存储在以下vector中
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Texture> textures;
            MaterialColors matColors;

            // Vertex
            for(unsigned int i=0; i<mesh->mNumVertices; i++) {
                // position、normal、texCoords
                Vertex vertex;  
                glm::vec3 tmp = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                vertex.Position = tmp;
                if(mesh->mNormals) {
                    tmp = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                    vertex.Normal = tmp;
                }
                else {
                    vertex.Normal = glm::vec3(0);
                }
                if(mesh->mTextureCoords[0]) {
                    glm::vec2 vec = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                    vertex.TexCoords = vec;
                } 
                else vertex.TexCoords = glm::vec2(0.0f, 0.0f);

                vertices.push_back(vertex);
            }
            // indice
            for(unsigned int i=0; i<mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for(unsigned int j=0; j<face.mNumIndices; j++) {
                    indices.push_back(face.mIndices[j]);
                }
            }
            // material
            if(mesh->mMaterialIndex >= 0) {
                aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex]; // mesh->mMaterialIndex也是索引
                std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
                textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
                std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
                textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

                // 颜色
                matColors = loadMaterialColors(material);
                matColors.hasTexture = (textures.size() > 0);
            }

            return Mesh(vertices, indices, textures, matColors);

        }
        std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
            std::vector<Texture> textures;
            for(unsigned int i=0; i<mat->GetTextureCount(type); i++) {
                aiString str;
                mat->GetTexture(type, i, &str); // 纹理的文件位置保存在str中
                bool skip = false;  // 纹理是否已经存在，防止重复加载
                for(unsigned int j=0; j<textures_loaded.size(); j++) {
                    if(std::strcmp(textures_loaded[j].path.C_Str(), str.C_Str()) == 0) {
                        textures.push_back(textures_loaded[j]);
                        skip = true;
                        break;
                    }
                }
                if(skip) continue;

                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), directory);
                texture.type = typeName;
                texture.path = str; 
                textures.push_back(texture);
                textures_loaded.push_back(texture); // 记录
            }
            return textures;
        }
        //  支持颜色
        MaterialColors loadMaterialColors(aiMaterial *mat) {
            MaterialColors colors;
            aiColor3D color;
            if(mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
                colors.ambient = glm::vec3(color.r, color.g, color.b);
            }
            else {
                colors.ambient = glm::vec3(0.1f);
            }
            if(mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                colors.diffuse = glm::vec3(color.r, color.g, color.b);
            }
            else {
                colors.diffuse = glm::vec3(0.0);
            }
            if(mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
                colors.specular = glm::vec3(color.r, color.g, color.b);
            }
            else {
                colors.specular = glm::vec3(0.0);
            }
            float shininess;
            if(mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                colors.shininess = shininess;
            }
            else colors.shininess = 32.0f;

            colors.hasTexture = false;
            return colors;
        }

        unsigned int TextureFromFile(const char *name, std::string directory) {
            stbi_set_flip_vertically_on_load(true); // reverse image
            std::string fullPath = directory + '/' + name;
            unsigned int textureID;
            glGenTextures(1, &textureID);
            int width, height, nrChannels;
            unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);
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
                return textureID;     
            }
            else {
                std::cout << "load error" << std::endl;
                return -1;
            }
        }   

};