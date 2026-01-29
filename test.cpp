#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

int main() {
    glm::mat3 data = glm::mat3(1.155, 2.309, 2.309,
                                2.309, 9.238, 6.928,
                                2.309, 6.928, 5.774);
    for(int i=0;i<3;i++) {
        float sum = 0;
        for(int j=0;j<3;j++) 
        sum += glm::exp(data[j][i]);
        // std::cout << data[j][i] << ' ';
        for(int j=0;j<3;j++) data[j][i] = glm::exp(data[j][i]) / sum;
    }
    for(int i=0;i <3;i++) {
        for(int j=0;j<3;j++) {
            std::cout << data[j][i] << ' ';
            
        }
        std::cout << std::endl;
    }
    return 0;
}