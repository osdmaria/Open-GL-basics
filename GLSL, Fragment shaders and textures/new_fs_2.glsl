#version 330
in vec3 color;
out vec4 fragColor;
uniform vec4 mousePos;

void main()
{
    float dx = gl_FragCoord.x - mousePos.x;
    float dy = gl_FragCoord.y - mousePos.y;
    float d = sqrt(dx * dx + dy * dy);
    float c = sin(d*0.4)*0.4 + 0.75;
    float period = 80.0; 
    float phase = mod(d / period, 1.0); 

   
    vec3 col;
    if (phase < 1.0 / 6.0)
        col = vec3(1.0, 0.0, 0.0); // Rouge
    else if (phase < 2.0 / 6.0)
        col = vec3(0.0, 1.0, 0.0); // Vert
    else if (phase < 3.0 / 6.0)
        col = vec3(0.0, 0.0, 1.0); // Bleu
    else if (phase < 4.0 / 6.0)
        col = vec3(1.0, 1.0, 0.0); // Jaune
    else if (phase < 5.0 / 6.0)
        col = vec3(0.0, 1.0, 1.0); // Cyan
    else
        col = vec3(1.0, 0.0, 1.0); // Magenta

    fragColor = vec4(col*c, 1.0);
}
