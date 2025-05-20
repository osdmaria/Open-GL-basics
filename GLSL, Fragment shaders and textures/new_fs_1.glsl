#version 330
in vec3 color;
out vec4 fragColor;
uniform vec4 mousePos;

void main()
{
    float dx = gl_FragCoord.x - mousePos.x,
          dy = gl_FragCoord.y - mousePos.y,
       //  d = sqrt(dx*dx + dy*dy),
       // La premiére distance d1 donne une courbe de forme lozange    
       d1 = abs(dx) + abs(dy),
       // La deuxième distance d2 donne une courbe de forme en forme de L   
       d_infini = max(dx, dy),
       // La troisième distance d3 donne une courbe de forme L dans la partie gauche en bas
       //et une courbe de la forme hexagonale en haut a droite  et de la forme / dans la partie haute gauche et basse droite 
       d3 = d1 + d_infini,
       // c = sin(d)*0.5 + 0.5;
          c = sin(d3*0.4)*0.25 + 0.75;
    fragColor = vec4 (color*c, 1.0);
}

