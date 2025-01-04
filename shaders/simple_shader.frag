#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec4 fragPosLightSpace;

// this is the output variable.
// the "layout" qualifier takes a location value.
// a fragment shader is capable of outputting to multiple different locations.
// the "out" qualifier specifies that this variable will be used as an output.
layout (location = 0) out vec4 outColor;

struct PointLight {
    vec4 position; // ignore w
    vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

// Shadow calculation function
float ShadowCalculation(vec4 fragPosLightSpace) {
    // Perform perspective divide (convert from clip space to NDC)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Get closest depth stored in shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Add bias to prevent shadow acne
    float bias = 0.005;
    
    // Check if current fragment is in shadow
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    // If projection is outside shadow map, don't cast shadow
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main() {
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);
        float cosAngIncidence = max(dot(surfaceNormal, directionToLight),0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;
        diffuseLight += intensity * cosAngIncidence;

        //specular lighting
        vec3 halfAngle = normalize(viewDirection + directionToLight);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0); // higher value = sharper highlight
        specularLight += intensity * blinnTerm;
    }

    outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);
    
    // Calculate shadow
    float shadow = ShadowCalculation(fragPosLightSpace);
    
    // Apply shadow to lighting
    vec3 lightingColor = (1.0 - shadow) * (diffuseLight + specularLight);
    
    outColor = vec4(fragColor * lightingColor, 1.0);
}