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
    uniform samplerCube shadowMap;

    uniform float near_plane;
    uniform float far_plane;
    uniform float user_area;

    float linearizeDepth(float z)
    {
        float z_n = 2.0 * z - 1.0;
        return 2.0 * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane));
    };

    // Return 1 for shadowed, 0 visible
    bool isShadowed(vec3 lightDir)
    {
        float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001) * far_plane;

        float z = linearizeDepth(texture(shadowMap, lightDir).r);
        return lightDistance - bias > z;
    }

    void main()
    {
        vec3 baseColor = texture(baseTexture, texCoords).rgb;

        if (length(lightPos.xy - worldPos.xy) > user_area)
            FragColor = vec4(baseColor, 1.0);
        else
        {
            vec3 lightDir = normalize(lightPos - worldPos);
            float normDif = max(dot(lightDir, normal), 0.0);

            vec3 lighting;
            // Render as visible only if it can be seen by light source
            if (normDif > 0.0 && isShadowed(-lightDir) == false)
                lighting = visibleColor.rgb * baseColor;
            else
                lighting = invisibleColor.rgb * baseColor;

            FragColor = vec4(lighting, 1.0);
        }
    }
)";

const char* depthVisualizerVert = R"(
    #version 330 core

    //in vec4 gl_Vertex;

    uniform mat4 osg_ModelViewMatrix;
    uniform mat4 osg_ViewMatrixInverse;
    uniform vec3 center;

    out vec3 texCoords;

    void main()
    {
        vec3 worldPos = (osg_ViewMatrixInverse * osg_ModelViewMatrix * osg_Vertex).xyz;
        texCoords = normalize(worldPos - center);
        gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
    }
)";

const char* depthVisualizerFrag = R"(
    #version 330 core
    #extension GL_NV_shadow_samplers_cube : enable
    out vec4 FragColor;

    in vec3 texCoords;

    uniform samplerCube depthMap;
    uniform samplerCube colorMap;

    const float near_plane = 0.1;
    const float far_plane = 1000.0;

    float LinearizeDepth(float z)
    {
        float z_n = 2.0 * z - 1.0;
        return 2.0 * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane));
    };

    void main()
    {
        // vec3 color = vec3(LinearizeDepth(textureCube(depthMap, texCoords).r)) / far_plane;
        vec3 color = vec3(textureCube(depthMap, texCoords).r);
        // vec3 color = vec3(textureCube(colorMap, texCoords));

        FragColor = vec4(color, 1.0);
    }
)";




#endif // VIEWSHEDSHADERS_H
