#pragma once

namespace s21::shaders {

// EDGES
constexpr const char* EDGE_VERT = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 mvp;
    void main() {
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

constexpr const char* EDGE_FRAG = R"(
    #version 330 core
    uniform vec3 color;
    uniform int dashed;
    out vec4 FragColor;
    void main() {
        if (dashed == 1) {
            if (int(mod(gl_FragCoord.x + gl_FragCoord.y, 10.0)) > 5) discard;
        }
        FragColor = vec4(color, 1.0);
    }
)";

// VERTICES
constexpr const char* POINT_VERT = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 mvp;
    uniform float pointSize;
    void main() {
        gl_Position = mvp * vec4(aPos, 1.0);
        gl_PointSize = pointSize;
    }
)";

constexpr const char* POINT_FRAG = R"(
    #version 330 core
    uniform vec3 color;
    uniform int circle;
    out vec4 FragColor;
    void main() {
        if (circle == 1) {
            vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
            if (dot(circCoord, circCoord) > 1.0) discard;
        }
        FragColor = vec4(color, 1.0);
    }
)";

// FLAT SHADING
constexpr const char* FLAT_VERT = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 mvp;
    uniform mat4 model;
    out vec3 FragPos;
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

constexpr const char* FLAT_FRAG = R"(
    #version 330 core
    in vec3 FragPos;
    uniform vec3 objectColor;
    uniform vec3 viewPos;
    uniform float ambientStrength;
    uniform float specularStrength;
    uniform float shininess;
    
    #define MAX_LIGHTS 5
    uniform int lightCount;
    uniform vec3 lightPos[MAX_LIGHTS];
    uniform vec3 lightColor[MAX_LIGHTS];
    uniform float lightIntensity[MAX_LIGHTS];
    
    out vec4 FragColor;
    void main() {
        vec3 xTangent = dFdx(FragPos);
        vec3 yTangent = dFdy(FragPos);
        vec3 norm = normalize(cross(xTangent, yTangent));
        
        vec3 ambient = ambientStrength * lightColor[0];
        vec3 result = ambient * objectColor;
        
        for (int i = 0; i < lightCount; i++) {
            vec3 lightDir = normalize(lightPos[i] - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor[i] * lightIntensity[i];
            
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor[i] * lightIntensity[i];
            
            result += (diffuse + specular) * objectColor;
        }
        FragColor = vec4(result, 1.0);
    }
)";

// GOURAUD SHADING
constexpr const char* GOURAUD_VERT = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    uniform mat4 mvp;
    uniform mat4 model;
    uniform vec3 objectColor;
    uniform vec3 viewPos;
    uniform float ambientStrength;
    uniform float specularStrength;
    uniform float shininess;
    
    #define MAX_LIGHTS 5
    uniform int lightCount;
    uniform vec3 lightPos[MAX_LIGHTS];
    uniform vec3 lightColor[MAX_LIGHTS];
    uniform float lightIntensity[MAX_LIGHTS];
    
    out vec3 LightingColor;
    void main() {
        vec3 FragPos = vec3(model * vec4(aPos, 1.0));
        vec3 Normal = mat3(transpose(inverse(model))) * aNormal;
        vec3 norm = normalize(Normal);
        
        vec3 ambient = ambientStrength * lightColor[0];
        vec3 result = ambient * objectColor;
        
        for (int i = 0; i < lightCount; i++) {
            vec3 lightDir = normalize(lightPos[i] - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor[i] * lightIntensity[i];
            
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor[i] * lightIntensity[i];
            
            result += (diffuse + specular) * objectColor;
        }
        
        LightingColor = result;
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

constexpr const char* GOURAUD_FRAG = R"(
    #version 330 core
    in vec3 LightingColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(LightingColor, 1.0);
    }
)";

// PHONG SHADING
constexpr const char* PHONG_VERT = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    uniform mat4 mvp;
    uniform mat4 model;
    out vec3 FragPos;
    out vec3 Normal;
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

constexpr const char* PHONG_FRAG = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    uniform vec3 objectColor;
    uniform vec3 viewPos;
    uniform float ambientStrength;
    uniform float specularStrength;
    uniform float shininess;
    
    #define MAX_LIGHTS 5
    uniform int lightCount;
    uniform vec3 lightPos[MAX_LIGHTS];
    uniform vec3 lightColor[MAX_LIGHTS];
    uniform float lightIntensity[MAX_LIGHTS];
    
    out vec4 FragColor;
    void main() {
        vec3 norm = normalize(Normal);
        vec3 ambient = ambientStrength * lightColor[0];
        vec3 result = ambient * objectColor;
        
        for (int i = 0; i < lightCount; i++) {
            vec3 lightDir = normalize(lightPos[i] - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor[i] * lightIntensity[i];
            
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor[i] * lightIntensity[i];
            
            result += (diffuse + specular) * objectColor;
        }
        FragColor = vec4(result, 1.0);
    }
)";

// TEXTURE SHADING
constexpr const char* TEXTURE_VERT = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    layout(location = 2) in vec2 aTexCoord;
    uniform mat4 mvp;
    uniform mat4 model;
    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord;
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoord = aTexCoord;
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

constexpr const char* TEXTURE_FRAG = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;
    uniform sampler2D textureSampler;
    uniform vec3 viewPos;
    uniform float ambientStrength;
    uniform float specularStrength;
    uniform float shininess;
    
    #define MAX_LIGHTS 5
    uniform int lightCount;
    uniform vec3 lightPos[MAX_LIGHTS];
    uniform vec3 lightColor[MAX_LIGHTS];
    uniform float lightIntensity[MAX_LIGHTS];
    
    out vec4 FragColor;
    void main() {
        vec3 objectColor = texture(textureSampler, TexCoord).rgb;
        vec3 norm = normalize(Normal);
        vec3 ambient = ambientStrength * lightColor[0];
        vec3 result = ambient * objectColor;
        
        for (int i = 0; i < lightCount; i++) {
            vec3 lightDir = normalize(lightPos[i] - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor[i] * lightIntensity[i];
            
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor[i] * lightIntensity[i];
            
            result += (diffuse + specular) * objectColor;
        }
        FragColor = vec4(result, 1.0);
    }
)";

}  // namespace s21::shaders