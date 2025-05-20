#version 330
in vec4 vPos;
in vec3 vCol;
out vec3 color;
uniform mat4 matMVP;

void main()
{
    gl_Position = matMVP * vPos;
    color = vCol;
}

