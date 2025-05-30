#version 330
in vec3 color;
out vec4 fragColor;
uniform vec4 mousePos;

void main()
{
    float dx = gl_FragCoord.x - mousePos.x,
          dy = gl_FragCoord.y - mousePos.y,
          d = sqrt(dx*dx + dy*dy),
       // c = sin(d)*0.5 + 0.5;
          c = sin(d*0.4)*0.25 + 0.75;
    fragColor = vec4 (color*c, 1.0);
}

