#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

struct Material
{
    sampler2D albedo;
    sampler2D normalMap;
    sampler2D specularMap;
    float shininess;
    vec3 specular;
};

struct Light
{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform Light light;
uniform Light light2;

uniform samplerCube shadowMap;
uniform float far_plane;

uniform vec3 viewPos;
uniform bool blinn;

float ShadowCalculation(vec3 fragPos);

void main()
{
    vec3 albedo = texture(material.albedo, fs_in.TexCoords).rgb;

    
    vec3 normal = texture(material.normalMap, fs_in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
    normal = normalize(fs_in.TBN * normal);

    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 lightDir = normalize(light.position - fs_in.FragPos);

    // ambient
    vec3 ambient = light.ambient * albedo;

    // diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = light.diffuse * diff * albedo;

    // specular
    float spec = 0.0;

    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }

    vec3 specular = light.specular * spec;

    
    float distance = length(light.position - fs_in.FragPos);

    float attenuation = 1.0 /
    (
        light.constant +
        light.linear * distance +
        light.quadratic * (distance * distance)
    );

    diffuse *= attenuation;
    specular *= attenuation;

    
    float shadow = ShadowCalculation(fs_in.FragPos);

    // light2 计算（紫色光源=======================================
    vec3 lightDir2 = normalize(light2.position - fs_in.FragPos);
    float diff2 = max(dot(lightDir2, normal), 0.0);
    vec3 diffuse2 = light2.diffuse * diff2 * albedo;

    float spec2 = 0.0;
    if(blinn)
    {
        vec3 halfwayDir2 = normalize(lightDir2 + viewDir);
        spec2 = pow(max(dot(normal, halfwayDir2), 0.0), material.shininess);
    }
    else
    {
        vec3 reflectDir2 = reflect(-lightDir2, normal);
        spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), material.shininess);
    }
    vec3 specular2 = light2.specular * spec2;

    float distance2 = length(light2.position - fs_in.FragPos);
    float attenuation2 = 1.0 / (light2.constant + light2.linear * distance2 + light2.quadratic * distance2 * distance2);
    diffuse2 *= attenuation2;
    specular2 *= attenuation2;
    //===================================================================

    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular) + diffuse2 + specular2;

    
    vec3 specularSample = texture(material.specularMap, fs_in.TexCoords).rgb;
    float emissiveMask = specularSample.r;  // B 通道是自发光遮罩
    emissiveMask = emissiveMask > 0.66 ? emissiveMask : 0.0;
    vec3 emissive = albedo * emissiveMask * 10.0;  // 强度倍数自行调整
    lighting += emissive;

    FragColor = vec4(lighting, 1.0);
    //FragColor = vec4(albedo * emissiveMask * 10.0, 1.0);

}



float ShadowCalculation(vec3 fragPos)
{
    vec3 fragToLight = fragPos - light.position;
    float closestDepth = texture(shadowMap, fragToLight).r * far_plane;
    float currentDepth = length(fragToLight);
    float bias = 0.05;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}