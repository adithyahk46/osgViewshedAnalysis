#ifndef VIEWSHEDSHADERS_H
#define VIEWSHEDSHADERS_H

const char* depthMapVert = R"(
    #version 330 core

    //in vec4 gl_Vertex;
    out float lightDistance;

    uniform mat4 osg_ModelViewProjectionMatrix;
    uniform mat4 osg_ViewMatrixInverse;
    uniform mat4 osg_ModelViewMatrix;
    uniform vec3 lightPos;
    uniform mat4 inverse_view;

    uniform float near_plane;
    uniform float far_plane;

    void main()
    {
        // get distance between fragment and light source
        vec3 worldPos = (inverse_view * osg_ModelViewMatrix * osg_Vertex).xyz;
        lightDistance = length(worldPos - lightPos);
        lightDistance = ((1 / lightDistance) - (1 / near_plane)) / (1 / far_plane - 1 / near_plane);

        gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
    }
)";

const char* depthMapFrag = R"(
    #version 330 core
    in float lightDistance;
    uniform float far_plane;

    void main()
    {
        // Mapping to [0, 1]
        gl_FragDepth = lightDistance;
    }
)";

const char* visibilityShaderVert = R"(
    #version 330 core

    //in vec4 gl_Vertex;
    //in vec3 gl_Normal;
    //in vec2 gl_MultiTexCoord0;

    uniform mat4 osg_ModelViewProjectionMatrix;
    uniform mat4 osg_ViewMatrixInverse;
    uniform mat4 osg_ModelViewMatrix;

    uniform vec3 lightPos;

    out vec3 worldPos;
    out vec3 normal;
    out vec2 texCoords;
    out float lightDistance;

    void main()
    {
        worldPos = (osg_ViewMatrixInverse * osg_ModelViewMatrix * osg_Vertex).xyz;
        lightDistance = length(worldPos - lightPos);

        normal = normalize( osg_Normal );
        texCoords = osg_MultiTexCoord0.xy;
        gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
    }
)";


const char* visibilityShaderFrag = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 worldPos;
    in vec3 normal;
    in vec2 texCoords;
    in float lightDistance;

    uniform vec3 lightPos;
    uniform vec4 visibleColor;
    uniform vec4 invisibleColor;

    uniform sampler2D baseTexture;

    uniform float near_plane;
    uniform float far_plane;
    uniform float user_area;

    uniform sampler2D shadowMap; // Changed to sampler2D
    uniform mat4 cameraVP;       // The new Uniform

    void main()
    {
        vec3 baseColor = texture(baseTexture, texCoords).rgb;

        // 1. Project world position into Camera's Screen Space
        vec4 projPos = cameraVP * vec4(worldPos, 1.0);
        vec3 projCoords = projPos.xyz / projPos.w; // Perspective divide

        // 2. Check if the point is inside the Camera's Frustum (FOV)
        // Since we used a bias matrix, the valid range is [0.0, 1.0]
        bool inFrustum = projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
                         projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
                         projCoords.z >= 0.0 && projCoords.z <= 1.0;

        float shadow = 0.0;
        if (inFrustum)
        {
            float closestDepth = texture(shadowMap, projCoords.xy).r;
            float currentDepth = projCoords.z;

            // Bias to prevent shadow acne
            float bias = 0.0005;
            shadow = (currentDepth - bias > closestDepth) ? 1.0 : 0.0;

            // Lighting calculation
            vec3 lightDir = normalize(lightPos - worldPos);
            float normDif = max(dot(normal, lightDir), 0.0);

            if (shadow < 0.5 && normDif > 0.1)
                FragColor = vec4(baseColor * visibleColor.rgb, 1.0);
            else
                FragColor = vec4(baseColor * invisibleColor.rgb, 1.0);
        }
        else
        {
            FragColor = vec4(baseColor, 1.0);
        }

    }
)";



#endif // VIEWSHEDSHADERS_H
