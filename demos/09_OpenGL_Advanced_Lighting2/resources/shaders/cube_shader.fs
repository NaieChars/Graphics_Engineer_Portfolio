#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Material
{
    sampler2D albedo;
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

uniform samplerCube shadowMap;
uniform float far_plane;

uniform vec3 viewPos;
uniform bool blinn;

float ShadowCalculation(vec3 fragPos);

void main()
{
    vec3 albedo = texture(material.albedo, fs_in.TexCoords).rgb;

    vec3 normal = normalize(fs_in.Normal);
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

    // ¾àÀëË¥¼õ
    float distance = length(light.position - fs_in.FragPos);

    float attenuation = 1.0 /
    (
        light.constant +
        light.linear * distance +
        light.quadratic * (distance * distance)
    );

    diffuse *= attenuation;
    specular *= attenuation;

    // ¼ÆËãÒõÓ°Öµ
    float shadow = ShadowCalculation(fs_in.FragPos);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    FragColor = vec4(lighting, 1.0);
}



float ShadowCalculation(vec3 fragPos)
{
    vec3 fragToLight = fragPos - light.position;
    float closestDepth = texture(shadowMap, fragToLight).r * far_plane;
    float currentDepth = length(fragToLight);
    float bias = 0.05;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}