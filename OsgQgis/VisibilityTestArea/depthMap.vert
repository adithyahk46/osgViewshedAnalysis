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