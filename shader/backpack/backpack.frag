#version 410 core
#define NR_POINT_LIGHTS 4

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

struct PointLight {
    vec3 position;
    vec3 direction;
    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

struct SpotLight {
    vec3 position;
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float cutOff_phi;
    float cutOff_gamma;
};

struct Material {
    // 使用纹理贴图
    sampler2D texture_diffuse1;  
    sampler2D texture_specular1; 
    float shininess;

    // 使用颜色
    bool useTexture;
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
};

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform vec3 lightColor;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 cameraDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 cameraDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 cameraDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 cameraDir = normalize(-FragPos);
    vec3 result;
    // direct light
    // vec3 result = CalcDirLight(dirLight, norm, cameraDir);
    // pointlight
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result = CalcPointLight(pointLights[i], norm, cameraDir);    
    // spotlight
    result += CalcSpotLight(spotLight, norm, cameraDir);    

    FragColor = vec4(result, 1.0);
    // FragColor = texture(material.texture_diffuse1, TexCoords);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 cameraDir)
{
    // ambient
    vec3 lightDir = normalize(light.direction);
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords)) * lightColor;
    // diffuse
    float diffuse_cos = max(dot(lightDir, normal), 0.0f);
    // float distance = float(dot(LightPos - FragPos, LightPos - FragPos));
    // float F = light.constant + light.linear * distance + light.quadratic * distance * distance;
    // vec3 diffuse = light.diffuse * vec3(texture(material.texture0_diffuse, TexCoords)) * diffuse_cos * lightColor / F;
    vec3 diffuse = light.diffuse * vec3(texture(material.texture_diffuse1, TexCoords)) * diffuse_cos * lightColor;
    // specular
    vec3 half_vec = normalize(lightDir + cameraDir);
    float specular_cos = pow(max(dot(normal, half_vec), 0.0f), material.shininess);
    vec3 specular = light.specular * vec3(texture(material.texture_specular1, TexCoords)) * specular_cos * lightColor;
    // vec3 specular = light.specular * vec3(texture(material.texture1_specular, TexCoords)) * lightColor;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 cameraDir) {
    vec3 ambientColor, diffuseColor, specularColor;
    if(material.useTexture) {
        diffuseColor = vec3(texture(material.texture_diffuse1, TexCoords));
        ambientColor = diffuseColor;
        specularColor = vec3(texture(material.texture_specular1, TexCoords));
    }
    else {
        ambientColor = material.ambientColor;
        diffuseColor = material.diffuseColor;
        specularColor = material.specularColor;
    }

    vec3 lightDir = normalize(light.position - FragPos);
    // ambient
    vec3 ambient = light.ambient * ambientColor * lightColor;
    // diffuse
    // vec3 lightDir = normalize(-light.direction);
    float diffuse_cos = max(dot(lightDir, normal), 0.0f);
    // float distance = float(dot(light.position - FragPos, light.position - FragPos));
    float distance = length(light.position - FragPos);
    float F = light.constant + light.linear * distance + light.quadratic * distance * distance;
    vec3 diffuse = light.diffuse * diffuseColor * diffuse_cos * lightColor / F;
    // specular
    vec3 half_vec = normalize(lightDir + cameraDir);
    float specular_cos = pow(max(dot(normal, half_vec), 0.0f), material.shininess);
    vec3 specular = light.specular * specularColor * specular_cos * lightColor / F;

    return ambient + diffuse + specular;
}
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 cameraDir) {

    vec3 ambientColor, diffuseColor, specularColor;
    if(material.useTexture) {
        diffuseColor = vec3(texture(material.texture_diffuse1, TexCoords));
        ambientColor = diffuseColor;
        specularColor = vec3(texture(material.texture_specular1, TexCoords));
    }
    else {
        ambientColor = material.ambientColor;
        diffuseColor = material.diffuseColor;
        specularColor = material.specularColor;
    }

    vec3 lightDir = normalize(light.position - FragPos);
    
    // intensity
    float theta = dot(lightDir, normalize(light.direction));
    float epsilon = light.cutOff_phi - light.cutOff_gamma;
    float intensity = 1.5f * clamp((theta - light.cutOff_gamma) / epsilon, 0.0, 1.0);  // 应当不影响ambient
    // float intensity = 1.0f;
    vec3 ambient = light.ambient * ambientColor * lightColor;
    float diffuse_cos = max(dot(lightDir, normal), 0.0f);
    // float distance = float(dot(light.position - FragPos, light.position - FragPos));
    float distance = length(light.position - FragPos);
    float F = light.constant + light.linear * distance + light.quadratic * distance * distance;
    vec3 diffuse = intensity * light.diffuse * diffuseColor * diffuse_cos * lightColor / F;
    // specular
    vec3 half_vec = normalize(lightDir + cameraDir);
    float specular_cos = pow(max(dot(normal, half_vec), 0.0f), material.shininess);
    vec3 specular = intensity * light.specular * specularColor * specular_cos * lightColor / F;

    // return ambient + diffuse;
    return ambient + diffuse + specular;
    
}