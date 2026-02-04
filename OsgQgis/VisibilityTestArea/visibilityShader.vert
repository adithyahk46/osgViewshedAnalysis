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
