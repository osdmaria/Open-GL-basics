/*
    Uniform Buffer Objects

    CC BY-SA Edouard.Thiel@univ-amu.fr - 08/02/2025
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <vector>

// Pour générer glad.h : https://glad.dav1d.de/
//   C/C++, gl 4.5, OpenGL, Core, extensions: add all, local files
#include "glad.h"

// Pour définir des matrices : module vmath.h du OpenGL Red Book
//   https://github.com/openglredbook/examples/blob/master/include/vmath.h
// avec bugfix: erreur de signe dans Ortho().
// provoque un warning avec -O2, supprimé avec -fno-strict-aliasing
// + ajout de calculs dans vmath-et.h
#include "vmath-et.h"

#include <GLFW/glfw3.h>

// Pour charger des images avec le module stb_image
#include "stb_image.h"


bool flag_fill = false;

//----------------------------- L O C A T I O N S -----------------------------

// Vertex attribute location imposées
enum VA_Locations{ VPOS_LOC = 0, VCOL_LOC = 1, VNOR_LOC = 2, VTEX_LOC = 3, LAST_LOC };

const char* get_vertex_attribute_name (VA_Locations loc)
{
    switch (loc) {
        case VPOS_LOC  : return "vPos";
        case VCOL_LOC  : return "vCol";
        case VNOR_LOC  : return "vNor";
        case VTEX_LOC  : return "vTex";
        default : return "";
    }
}


//----------------------------------- U B O -----------------------------------

// Type avec alignement GLSL std140 aligné sur 16 octets
struct UBO_Uniforms {
    alignas(16) vmath::mat4 matProj;
    alignas(16) vmath::mat4 matCam;
    alignas(16) vmath::vec4 mousePos;
    alignas(16) GLfloat time;
};

const GLint UBO_BINDING_POINT = 0;



//------------------------------ W I R E   C U B E ----------------------------

class WireCube
{
    bool m_is_white;
    GLfloat m_radius;
    GLuint m_VAO_id, m_VBO_id, m_EBO_id;

public:
    WireCube (bool is_white, GLfloat radius)
        : m_is_white {is_white}, m_radius {radius}
    {
        GLfloat r = m_radius;

        // Positions
        GLfloat positions[] = {
           -r, -r, -r,  // P0                 6 ------- 7
            r, -r, -r,  // P1               / |       / |
           -r,  r, -r,  // P2             /   |     /   |
            r,  r, -r,  // P3           2 ------- 3     |
           -r, -r,  r,  // P4           |     4 --|---- 5
            r, -r,  r,  // P5           |   /     |   /
           -r,  r,  r,  // P6           | /       | /
            r,  r,  r,  // P7           0 ------- 1
        };

        // Indices : positions, couleurs
        GLint indexes[] = {
            0, 1,   0, 0,
            2, 3,   0, 0,
            4, 5,   0, 0,
            6, 7,   0, 0,
            0, 2,   1, 1,
            1, 3,   1, 1,
            4, 6,   1, 1,
            5, 7,   1, 1,
            0, 4,   2, 2,
            1, 5,   2, 2,
            2, 6,   2, 2,
            3, 7,   2, 2
        };

        // Couleurs
        GLfloat colors_rgb[] = {
            1.0, 0.0, 0.0,  // C0       C1  C2
            0.0, 1.0, 0.0,  // C1       | /
            0.0, 0.0, 1.0   // C2       + --C0
        };
        GLfloat colors_white[] = {
            1.0, 1.0, 1.0,  // C0
            1.0, 1.0, 1.0,  // C1
            1.0, 1.0, 1.0   // C2
        };
        GLfloat *colors = (is_white) ? colors_white : colors_rgb;

        // Création d'une structure de données à plat
        std::vector<GLfloat> vertices;
        for (int i = 0; i < 12*4; i+=4) {
            for (int j = 0; j < 3; j++)
                vertices.push_back (positions[indexes[i+0]*3+j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back (colors[indexes[i+2]*3+j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back (positions[indexes[i+1]*3+j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back (colors[indexes[i+3]*3+j]);
        }

        // Création du VAO
        glCreateVertexArrays (1, &m_VAO_id);
        glBindVertexArray (m_VAO_id);

        // Création du VBO pour les positions et couleurs
        glGenBuffers (1, &m_VBO_id);
        glBindBuffer (GL_ARRAY_BUFFER, m_VBO_id);

        // Copie le buffer dans la mémoire du serveur
        glBufferData (GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), 
            vertices.data(), GL_STATIC_DRAW);

        // VAA associant les données à la variable vPos du shader, avec l'offset 0
        glVertexAttribPointer (VPOS_LOC, 3, GL_FLOAT, GL_FALSE, 
            6*sizeof(GLfloat), reinterpret_cast<void*>(0*sizeof(GLfloat)));
        glEnableVertexAttribArray (VPOS_LOC);  

        // VAA associant les données à la variable vCol du shader, avec l'offset 3
        glVertexAttribPointer (VCOL_LOC, 3, GL_FLOAT, GL_FALSE, 
            6*sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(GLfloat)));
        glEnableVertexAttribArray (VCOL_LOC);  

        glBindVertexArray (0);  // désactive le VAO courant m_VAO_id
    }


    ~WireCube()
    {
        glDeleteBuffers (1, &m_VBO_id);
        glDeleteVertexArrays (1, &m_VAO_id);
    }


    void draw()
    {
        glBindVertexArray(m_VAO_id);
        glDrawArrays (GL_LINES, 0, 24);
        glBindVertexArray (0);
    }

}; // WireCube




//------------------------------ R O U E ----------------------------

class RoueNor{
    GLuint m_VAO_id, m_VBO_id;
    GLint m_vPos_loc, m_vCol_loc, m_vNor_loc;
    int m_nb_dents;          
    double m_r_trou;   
    double m_r_roue;        
    double m_h_dent;        
    double m_coul_r, m_coul_v, m_coul_b; 
    double m_ep_roue;      

public:
RoueNor (GLint vPos_loc, GLint vCol_loc, GLint vNor_loc, int nb_dents, double r_trou, double r_roue, double h_dent, 
         double coul_r, double coul_v, double coul_b, double ep_roue)
        : m_vPos_loc {vPos_loc}, m_vCol_loc {vCol_loc}, m_vNor_loc {vNor_loc}, 
        m_nb_dents{nb_dents},
        m_r_trou {r_trou},
        m_r_roue {r_roue},
        m_h_dent {h_dent},
        m_coul_r {coul_r},
        m_coul_v {coul_v},
        m_coul_b {coul_b},
        m_ep_roue {ep_roue}
        
    {
        std::vector<GLfloat> positions; // positions pour les 2 faces


        for (int index = 1; index<= m_nb_dents; index++){
            GLfloat alpha = (2 * M_PI) / m_nb_dents; // Equ à 360 / nb_dents
            GLfloat angle_base = index * alpha; 

            GLfloat alphaA = angle_base;                        // Point A
            GLfloat alphaB = angle_base;                        // Point B
            GLfloat alphaC = angle_base + alpha / 4;       // Point C
            GLfloat alphaD = angle_base + 2 * alpha / 4;       // Point D
            GLfloat alphaE = angle_base + 3 * alpha / 4;   // Point E
            GLfloat alphaH = angle_base + 2 * alpha / 4;       // Point H
            GLfloat alphaG = angle_base + alpha;           // Point G


            GLfloat r_roue_inter = m_r_roue - m_h_dent / 2;     // Rayon pour B, F, C
            GLfloat r_roue_exter = m_r_roue + m_h_dent / 2;     // Rayon pour D, E

            GLfloat xA = m_r_trou * cos(alphaA), yA = m_r_trou * sin(alphaA);       // Point A
            double xH = m_r_trou * cos(alphaH), yH = m_r_trou * sin(alphaH); // Point H
            GLfloat xB = r_roue_inter * cos(alphaB), yB = r_roue_inter * sin(alphaB); // Point B
            GLfloat xC = r_roue_inter * cos(alphaC), yC = r_roue_inter * sin(alphaC); // Point C
            GLfloat xD = r_roue_exter * cos(alphaD), yD = r_roue_exter * sin(alphaD); // Point D
            GLfloat xE = r_roue_exter * cos(alphaE), yE = r_roue_exter * sin(alphaE); // Point E
            GLfloat xG = m_r_trou * cos(alphaG), yG = m_r_trou * sin(alphaG);    

            GLfloat xI = (xA + xH)/2 , yI = (yA + yH)/2;
            GLfloat xJ = (xH + xG)/2 , yJ = (yH + yG)/2;
            
            positions.push_back(xA); positions.push_back(yA); 
            positions.push_back(xB); positions.push_back(yB); 
            positions.push_back(xI); positions.push_back(yI); 
            positions.push_back(xC); positions.push_back(yC); 
            positions.push_back(xH); positions.push_back(yH); 
            positions.push_back(xD); positions.push_back(yD); 
            positions.push_back(xJ); positions.push_back(yJ); 
            positions.push_back(xE); positions.push_back(yE); 
        }

        std::vector<GLint> ind_pos;
        for(int i =0; i<8*m_nb_dents; i++){
            ind_pos.push_back(i);
        }              
        ind_pos.push_back(0); // pour A et B
        ind_pos.push_back(1);

        std::vector<GLfloat> positions1; // positions pour Les faces du trou
        std::vector<GLfloat> normals; // normales des faces du trou

       // Calcule positions des sommets constituant le trou
       for (int index = 1; index <= m_nb_dents; index++) {
            GLfloat alpha = (2 * M_PI) / m_nb_dents; // Équivalent à 360 / nb_dents
            GLfloat angle_base = index * alpha;

            GLfloat alphaA = angle_base; // Point A
            GLfloat alphaH = angle_base + 2 * alpha / 4; // Point H

            GLfloat xA = m_r_trou * cos(alphaA), yA = m_r_trou * sin(alphaA); // Point A
            GLfloat xH = m_r_trou * cos(alphaH), yH = m_r_trou * sin(alphaH); // Point H

            // Ajout des sommets A', A, H', H
            positions1.push_back(xA); positions1.push_back(yA); positions1.push_back(-m_ep_roue / 2); // A'
            positions1.push_back(xA); positions1.push_back(yA); positions1.push_back(m_ep_roue / 2);  // A
            positions1.push_back(xH); positions1.push_back(yH); positions1.push_back(-m_ep_roue / 2); // H'
            positions1.push_back(xH); positions1.push_back(yH); positions1.push_back(m_ep_roue / 2);  // H

            // Ajout des normales pour A', A, H', H
            normals.push_back(-xA); normals.push_back(-yA); normals.push_back(0); // A'
            normals.push_back(-xA); normals.push_back(-yA); normals.push_back(0); // A
            normals.push_back(-xH); normals.push_back(-yH); normals.push_back(0); // H'
            normals.push_back(-xH); normals.push_back(-yH); normals.push_back(0); // H
    }

        // Ajout des sommets A', A à la fin pour fermer le triangle strip
        GLfloat alpha = (2 * M_PI) / m_nb_dents; 
        GLfloat angle_base = 1 * alpha;

        GLfloat alphaA = angle_base; // Point A
        GLfloat xA = m_r_trou * cos(alphaA), yA = m_r_trou * sin(alphaA); // Point A

        positions1.push_back(xA); positions1.push_back(yA); positions1.push_back(-m_ep_roue / 2); // A'
        positions1.push_back(xA); positions1.push_back(yA); positions1.push_back(m_ep_roue / 2);  // A

        normals.push_back(-xA); normals.push_back(-yA); normals.push_back(0); // A'
        normals.push_back(-xA); normals.push_back(-yA); normals.push_back(0); // A


        std::vector<GLfloat> positions2; // positions pour Les faces du pourtour
        std::vector<GLfloat> normals1;

        // Calcule positions des sommets constituant le pourtour
        for (int index = 1; index <= m_nb_dents; index++) {
            GLfloat alpha = (2 * M_PI) / m_nb_dents; // Angle entre les dents
            GLfloat angle_base = index * alpha;

            GLfloat alphaB = angle_base;
            GLfloat alphaC = angle_base + alpha / 4;
            GLfloat alphaD = angle_base + 2 * alpha / 4;
            GLfloat alphaE = angle_base + 3 * alpha / 4;
            GLfloat alphaF = angle_base + alpha;   // Point F

            GLfloat r_roue_inter = m_r_roue - m_h_dent / 2; 
            GLfloat r_roue_exter = m_r_roue + m_h_dent / 2; 

            GLfloat xB = r_roue_inter * cos(alphaB), yB = r_roue_inter * sin(alphaB); // Point B
            GLfloat xC = r_roue_inter * cos(alphaC), yC = r_roue_inter * sin(alphaC); // Point C
            GLfloat xD = r_roue_exter * cos(alphaD), yD = r_roue_exter * sin(alphaD); // Point D
            GLfloat xE = r_roue_exter * cos(alphaE), yE = r_roue_exter * sin(alphaE); // Point E
            GLfloat xF = r_roue_inter * cos(alphaF), yF = r_roue_inter * sin(alphaF); // Point F

            // Points B', C', D', E', F' (décalés en z)
            GLfloat xB_prime = xB, yB_prime = yB, zB_prime = -m_ep_roue / 2;
            GLfloat xC_prime = xC, yC_prime = yC, zC_prime = -m_ep_roue / 2;
            GLfloat xD_prime = xD, yD_prime = yD, zD_prime = -m_ep_roue / 2;
            GLfloat xE_prime = xE, yE_prime = yE, zE_prime = -m_ep_roue / 2;
            GLfloat xF_prime = xF, yF_prime = yF, zF_prime = -m_ep_roue / 2;

            // Normales pour chaque face
            // Face BB'CC'
            GLfloat nx1 = -(yB - yC), ny1 = xB - xC, nz1 = 0;
            // Face CC'DD'
            GLfloat nx2 = -(yC - yD), ny2 = xC - xD, nz2 = 0;
            // Face DD'EE'
            GLfloat nx3 = -(yD - yE), ny3 = xD - xE, nz3 = 0;
            // Face EE'FF'
            GLfloat nx4 = -(yE - yF), ny4 = xE - xF, nz4 = 0;

            // Ajout des sommets et normales pour chaque face
            // Face BB'CC'
            addQuad(positions2, normals1, xB, yB, m_ep_roue / 2, xB_prime, yB_prime, zB_prime,
                    xC, yC, m_ep_roue / 2, xC_prime, yC_prime, zC_prime, nx1, ny1, nz1);

            // Face CC'DD'
            addQuad(positions2, normals1, xC, yC, m_ep_roue / 2, xC_prime, yC_prime, zC_prime,
                    xD, yD, m_ep_roue / 2, xD_prime, yD_prime, zD_prime, nx2, ny2, nz2);

            // Face DD'EE'
            addQuad(positions2, normals1, xD, yD, m_ep_roue / 2, xD_prime, yD_prime, zD_prime,
                    xE, yE, m_ep_roue / 2, xE_prime, yE_prime, zE_prime, nx3, ny3, nz3);

            // Face EE'FF'
            addQuad(positions2, normals1, xE, yE, m_ep_roue / 2, xE_prime, yE_prime, zE_prime,
                    xF, yF, m_ep_roue / 2, xF_prime, yF_prime, zF_prime, nx4, ny4, nz4);
        }

        std::vector<GLfloat> vertices;
        // la première face
        for (int i = 0; i < 8*m_nb_dents + 2; i++) {
            // Positions sommets
            for (int j = 0; j < 2; j++)
                vertices.push_back (positions[ind_pos[i]*2+j]);
            vertices.push_back(m_ep_roue/2); // jai sortie le z pour que je le mette à - pour la prochaine face
            // Couleurs sommets
            vertices.push_back(m_coul_r);
            vertices.push_back(m_coul_v);
            vertices.push_back(m_coul_b);
            // Normales sommets
            vertices.push_back (0);
            vertices.push_back (0);
            vertices.push_back (1);
        }
        // la deuxième face
        for (int i = 0; i < 8*m_nb_dents + 2; i++) {
        // Positions sommets
            for (int j = 0; j < 2; j++)
                vertices.push_back (positions[ind_pos[i]*2+j]);
            vertices.push_back(- m_ep_roue/2);
                // Couleurs sommets
            vertices.push_back(m_coul_r);
            vertices.push_back(m_coul_v);
            vertices.push_back(m_coul_b);
            // Normales sommets
            vertices.push_back (0);
            vertices.push_back (0);
            vertices.push_back (-1);
        }
        
        // trou
    for (size_t i = 0; i < positions1.size() / 3; i++) {
        // positions1
        vertices.push_back(positions1[i * 3]);
        vertices.push_back(positions1[i * 3 + 1]);
        vertices.push_back(positions1[i * 3 + 2]);

        // Couleurs
        vertices.push_back(m_coul_r);
        vertices.push_back(m_coul_v);
        vertices.push_back(m_coul_b);

        // Normales
        vertices.push_back(normals[i * 3]);
        vertices.push_back(normals[i * 3 + 1]);
        vertices.push_back(normals[i * 3 + 2]);
    }
       
    // pourtour
    for (size_t i = 0; i < positions2.size() / 3; i++) {
        // Positions
        vertices.push_back(positions2[i * 3]);
        vertices.push_back(positions2[i * 3 + 1]);
        vertices.push_back(positions2[i * 3 + 2]);

        // Couleurs
        vertices.push_back(m_coul_r);
        vertices.push_back(m_coul_v);
        vertices.push_back(m_coul_b);

        // Normales
        vertices.push_back(normals1[i * 3]);
        vertices.push_back(normals1[i * 3 + 1]);
        vertices.push_back(normals1[i * 3 + 2]);
    }

        // Création du VAO
        glCreateVertexArrays (1, &m_VAO_id);
        glBindVertexArray (m_VAO_id);

        // Création du VBO pour les positions et couleurs
        glGenBuffers (1, &m_VBO_id);
        glBindBuffer (GL_ARRAY_BUFFER, m_VBO_id);

        // Copie le buffer dans la mémoire du serveur
        glBufferData (GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), 
            vertices.data(), GL_STATIC_DRAW);

        // VAA associant les données à la variable vPos du shader, avec l'offset 0
        glVertexAttribPointer (m_vPos_loc, 3, GL_FLOAT, GL_FALSE, 
            9*sizeof(GLfloat), reinterpret_cast<void*>(0*sizeof(GLfloat)));
        glEnableVertexAttribArray (m_vPos_loc);  

        // VAA associant les données à la variable vCol du shader, avec l'offset 3
        glVertexAttribPointer (m_vCol_loc, 3, GL_FLOAT, GL_FALSE, 
            9*sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(GLfloat)));
        glEnableVertexAttribArray (m_vCol_loc);  

        // VAA associant les données à la variable vNor du shader, avec l'offset 6
        glVertexAttribPointer (m_vNor_loc, 3, GL_FLOAT, GL_FALSE, 
            9*sizeof(GLfloat), reinterpret_cast<void*>(6*sizeof(GLfloat)));
        glEnableVertexAttribArray (m_vNor_loc);  

        glBindVertexArray (0);  // désactive le VAO courant m_VAO_id
        
    }

    ~RoueNor()
    {
        glDeleteBuffers (1, &m_VBO_id);
        glDeleteVertexArrays (1, &m_VAO_id);
    }

    void draw ()
    {
        glPolygonMode (GL_FRONT_AND_BACK, flag_fill ? GL_FILL : GL_LINE);
        glBindVertexArray (m_VAO_id);
        glDrawArrays (GL_TRIANGLE_STRIP, 0, 8*m_nb_dents + 2); // dessine face 1
        glDrawArrays (GL_TRIANGLE_STRIP, 8*m_nb_dents + 2, 2* (8*m_nb_dents + 2)); // dessine face 2
        glDrawArrays(GL_TRIANGLE_STRIP,8*m_nb_dents + 2 + 2* (8*m_nb_dents + 2), 4 * m_nb_dents + 2); // Dessine le pourtour du trou

        int offset = 2 * (8 * m_nb_dents + 2) + (4 * m_nb_dents + 2); // Offset après le pourtour du trou
        for (int i = 0; i < m_nb_dents; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, offset + i * 16, 16); // Chaque face a 4 sommets
        }

        glBindVertexArray (0);
    }

    private:
    // une fonction pour ajouter les faces vu que c'est trop de tout reecrire pour le reste xD
    void addQuad(std::vector<GLfloat>& positions, std::vector<GLfloat>& normals,
        GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2,
        GLfloat x3, GLfloat y3, GLfloat z3, GLfloat x4, GLfloat y4, GLfloat z4,
        GLfloat nx, GLfloat ny, GLfloat nz) {
        positions.push_back(x1); positions.push_back(y1); positions.push_back(z1);  
        positions.push_back(x2); positions.push_back(y2); positions.push_back(z2);  
        positions.push_back(x3); positions.push_back(y3); positions.push_back(z3);  
        positions.push_back(x4); positions.push_back(y4); positions.push_back(z4);  

        for (int i = 0; i < 4; i++) {
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        }
    }

};

//------------------------------ C Y L I N D R E ----------------------------


class Cylindre {
    GLuint m_VAO_id, m_VBO_id; // IDs pour le VAO et le VBO
    GLint m_vPos_loc, m_vCol_loc; // Localisations des attributs dans le shader

    double m_ep_cyl;
    double m_r_cyl;
    int m_nb_fac;
    float m_coul_r, m_coul_v, m_coul_b;

    std::vector<GLfloat> m_vertices_and_colors; // Contient les sommets et couleurs

public:
    Cylindre(double ep_cyl, double r_cyl, int nb_fac, float coul_r, float coul_v, float coul_b, GLint vPos_loc, GLint vCol_loc)
        : m_ep_cyl(ep_cyl), m_r_cyl(r_cyl), m_nb_fac(nb_fac),
          m_coul_r(coul_r), m_coul_v(coul_v), m_coul_b(coul_b),
          m_vPos_loc(vPos_loc), m_vCol_loc(vCol_loc) {
        generateVerticesAndColors();
        setupBuffers();
    }

    ~Cylindre() {
        glDeleteBuffers(1, &m_VBO_id);
        glDeleteVertexArrays(1, &m_VAO_id);
    }

    void generateVerticesAndColors() {
        m_vertices_and_colors.clear();

        // Générer sommets et couleurs pour les deux faces et les facettes
        for (int side = -1; side <= 1; side += 2) {
            m_vertices_and_colors.push_back(0.0f); // Centre
            m_vertices_and_colors.push_back(0.0f);
            m_vertices_and_colors.push_back(side * m_ep_cyl / 2);
            m_vertices_and_colors.push_back(m_coul_r);
            m_vertices_and_colors.push_back(m_coul_v);
            m_vertices_and_colors.push_back(m_coul_b);

            for (int i = 0; i <= m_nb_fac; ++i) {
                float angle = 2.0 * M_PI * i / m_nb_fac;
                float x = m_r_cyl * cos(angle);
                float y = m_r_cyl * sin(angle);
                float z = side * m_ep_cyl / 2;

                m_vertices_and_colors.push_back(x);
                m_vertices_and_colors.push_back(y);
                m_vertices_and_colors.push_back(z);
                m_vertices_and_colors.push_back(m_coul_r);
                m_vertices_and_colors.push_back(m_coul_v);
                m_vertices_and_colors.push_back(m_coul_b);
            }
        }

        for (int i = 0; i <= m_nb_fac; ++i) {
            float angle = 2.0 * M_PI * i / m_nb_fac;
            float x = m_r_cyl * cos(angle);
            float y = m_r_cyl * sin(angle);

            m_vertices_and_colors.push_back(x);
            m_vertices_and_colors.push_back(y);
            m_vertices_and_colors.push_back(-m_ep_cyl / 2);
            m_vertices_and_colors.push_back(m_coul_r * 0.8f);
            m_vertices_and_colors.push_back(m_coul_v * 0.8f);
            m_vertices_and_colors.push_back(m_coul_b * 0.8f);

            m_vertices_and_colors.push_back(x);
            m_vertices_and_colors.push_back(y);
            m_vertices_and_colors.push_back(m_ep_cyl / 2);
            m_vertices_and_colors.push_back(m_coul_r * 0.8f);
            m_vertices_and_colors.push_back(m_coul_v * 0.8f);
            m_vertices_and_colors.push_back(m_coul_b * 0.8f);
        }
    }

    void setupBuffers() {
        glCreateVertexArrays(1, &m_VAO_id);
        glBindVertexArray(m_VAO_id);

        glGenBuffers(1, &m_VBO_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO_id);

        glBufferData(GL_ARRAY_BUFFER, m_vertices_and_colors.size() * sizeof(GLfloat), m_vertices_and_colors.data(), GL_STATIC_DRAW);

        // Attributs pour les positions
        glVertexAttribPointer(m_vPos_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(m_vPos_loc);

        // Attributs pour les couleurs
        glVertexAttribPointer(m_vCol_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(m_vCol_loc);

        glBindVertexArray(0);
    }

    void draw() {
        glBindVertexArray(m_VAO_id);

        // Dessiner la facette avant (côté -ep_cyl/2)
        glPolygonMode (GL_FRONT_AND_BACK, flag_fill ? GL_FILL : GL_LINE);
        glDrawArrays(GL_TRIANGLE_FAN, 0, m_nb_fac + 2);

        // Dessiner la facette arrière (côté +ep_cyl/2)
        glDrawArrays(GL_TRIANGLE_FAN, m_nb_fac + 2, m_nb_fac + 2);

        // Dessiner les facettes latérales
        glDrawArrays(GL_TRIANGLE_STRIP, 2 * (m_nb_fac + 2), 2 * (m_nb_fac + 1));

        glBindVertexArray(0);
    }
};


//------------------------------ P E D A L E  ----------------------------

class Pedale {
    GLfloat m_larg, m_long, m_haut, m_chanf;
    GLfloat m_coul_r, m_coul_v, m_coul_b;
    GLuint m_VAO_id, m_VBO_id;
    GLint m_vPos_loc, m_vCol_loc;

public:
    Pedale(GLfloat larg, GLfloat long_, GLfloat haut, GLfloat chanf, 
           GLfloat coul_r, GLfloat coul_v, GLfloat coul_b,
           GLint vPos_loc, GLint vCol_loc)
        : m_larg{larg}, m_long{long_}, m_haut{haut}, m_chanf{chanf},
          m_coul_r{coul_r}, m_coul_v{coul_v}, m_coul_b{coul_b},
          m_vPos_loc{vPos_loc}, m_vCol_loc{vCol_loc} {
        
        // les positions des sommets
        GLfloat positions[] = {
            // Devant
            -m_larg/2+m_chanf, m_haut/2, m_long/2, // A 
            m_larg/2-m_chanf ,m_haut/2, m_long/2, //B 2
            - m_larg/2, m_haut/2 - m_chanf,m_long/2, //H 3
           m_larg/2, m_haut/2 - m_chanf,m_long/2, //C 4
            -m_larg/2, -m_haut/2 + m_chanf ,m_long/2, //G 5
            m_larg/2, -m_haut/2 + m_chanf ,m_long/2, //D 6
            -m_larg/2+m_chanf, - m_haut/2,m_long/2, //F 7
            m_larg/2-m_chanf, - m_haut/2,m_long/2, // E 8
            
            // Derriere
            -m_larg/2+m_chanf, m_haut/2, -m_long/2, // Aprime 9
            m_larg/2-m_chanf ,m_haut/2, -m_long/2, //Bprime 10
            - m_larg/2, m_haut/2 - m_chanf, -m_long/2, //Hprime 11
           m_larg/2, m_haut/2 - m_chanf, -m_long/2, //Cprime 12
            -m_larg/2, -m_haut/2 + m_chanf , -m_long/2, //Gprime 13
            m_larg/2, -m_haut/2 + m_chanf , -m_long/2, //Dprime 14
            -m_larg/2+m_chanf, - m_haut/2, -m_long/2, //Fprime 15
            m_larg/2-m_chanf, - m_haut/2, -m_long/2, // Eprime 16

            //Connecter les deux:
            m_larg/2-m_chanf ,m_haut/2, m_long/2, //B 2
            m_larg/2-m_chanf ,m_haut/2, -m_long/2, //Bprime 10
            m_larg/2, m_haut/2 - m_chanf,m_long/2, //C 4
            m_larg/2, m_haut/2 - m_chanf, -m_long/2, //Cprime 12
            m_larg/2, -m_haut/2 + m_chanf ,m_long/2, //D 6
            m_larg/2, -m_haut/2 + m_chanf , -m_long/2, //Dprime 14
            m_larg/2-m_chanf, - m_haut/2,m_long/2, // E 8
            m_larg/2-m_chanf, - m_haut/2, -m_long/2, // Eprime 16
            -m_larg/2+m_chanf, - m_haut/2,m_long/2, //F 7
            -m_larg/2+m_chanf, - m_haut/2, -m_long/2, //Fprime 15
            -m_larg/2, -m_haut/2 + m_chanf ,m_long/2, //G 5
            -m_larg/2, -m_haut/2 + m_chanf , -m_long/2, //Gprime 13
            - m_larg/2, m_haut/2 - m_chanf,m_long/2, //H 3
            - m_larg/2, m_haut/2 - m_chanf, -m_long/2, //Hprime 11
            -m_larg/2+m_chanf, m_haut/2, m_long/2, // A 1
            -m_larg/2+m_chanf, m_haut/2, -m_long/2, // Aprime 9
            m_larg/2-m_chanf ,m_haut/2, m_long/2, //B 2
            m_larg/2-m_chanf ,m_haut/2, -m_long/2, //Bprime 10
        };

        // structure pour les sommets
        std::vector<GLfloat> vertices;
        for (int i =0 ; i<16; i+=1){
            for(int j=0; j<3; j++){
                vertices.push_back(positions[i*3+j]);
            }
            vertices.push_back(m_coul_r);
            vertices.push_back(m_coul_v);
            vertices.push_back(m_coul_b);
        }
        for (int i =16 ; i<34; i+=1){
            for(int j=0; j<3; j++){
                vertices.push_back(positions[i*3+j]);
            }
            vertices.push_back(m_coul_r * 0.7);
            vertices.push_back(m_coul_v * 0.7);
            vertices.push_back(m_coul_b * 0.7);
        }



        glCreateVertexArrays(1, &m_VAO_id);
        glBindVertexArray(m_VAO_id);

        glGenBuffers(1, &m_VBO_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO_id);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), 
                     vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(m_vPos_loc, 3, GL_FLOAT, GL_FALSE, 
                              6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(m_vPos_loc);

        glVertexAttribPointer(m_vCol_loc, 3, GL_FLOAT, GL_FALSE, 
                              6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(m_vCol_loc);

        glBindVertexArray(0);
    }

    ~Pedale() {
        glDeleteBuffers(1, &m_VBO_id);
        glDeleteVertexArrays(1, &m_VAO_id);
    }

    void draw() {
        glPolygonMode (GL_FRONT_AND_BACK, flag_fill ? GL_FILL : GL_LINE);
        glBindVertexArray(m_VAO_id);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
        glDrawArrays(GL_TRIANGLE_STRIP, 8, 8);
        glDrawArrays(GL_TRIANGLE_STRIP, 16, 18);
        glBindVertexArray(0);


    }
};



//------------------------------ B O I T E  ----------------------------


class Boite
{
    GLfloat m_width, m_height, m_depth;
    GLuint m_VAO_id, m_VBO_id, m_EBO_id;

public:
    Boite(GLfloat width, GLfloat height, GLfloat depth)
        : m_width{width}, m_height{height}, m_depth{depth}
    {
        
        GLfloat halfWidth = m_width / 2.0f;
        GLfloat halfHeight = m_height / 2.0f;
        GLfloat halfDepth = m_depth / 2.0f;

       
        GLfloat positions[] = {
            // Face avant
            -halfWidth, -halfHeight,  halfDepth,  // 0
             halfWidth, -halfHeight,  halfDepth,  // 1
             halfWidth,  halfHeight,  halfDepth,  // 2
            -halfWidth,  halfHeight,  halfDepth,  // 3

            // Face arrière
            -halfWidth, -halfHeight, -halfDepth,  // 4
             halfWidth, -halfHeight, -halfDepth,  // 5
             halfWidth,  halfHeight, -halfDepth,  // 6
            -halfWidth,  halfHeight, -halfDepth,  // 7

            // Face gauche
            -halfWidth, -halfHeight, -halfDepth,  // 8
            -halfWidth, -halfHeight,  halfDepth,  // 9
            -halfWidth,  halfHeight,  halfDepth,  // 10
            -halfWidth,  halfHeight, -halfDepth,  // 11

            // Face droite
             halfWidth, -halfHeight, -halfDepth,  // 12
             halfWidth, -halfHeight,  halfDepth,  // 13
             halfWidth,  halfHeight,  halfDepth,  // 14
             halfWidth,  halfHeight, -halfDepth,  // 15

            // Face supérieure
            -halfWidth,  halfHeight,  halfDepth,  // 16
             halfWidth,  halfHeight,  halfDepth,  // 17
             halfWidth,  halfHeight, -halfDepth,  // 18
            -halfWidth,  halfHeight, -halfDepth,  // 19

            // Face inférieure
            -halfWidth, -halfHeight,  halfDepth,  // 20
             halfWidth, -halfHeight,  halfDepth,  // 21
             halfWidth, -halfHeight, -halfDepth,  // 22
            -halfWidth, -halfHeight, -halfDepth   // 23
        };

        
        GLuint indices[] = {
            // Face avant
            0, 1, 2,  0, 2, 3,

            // Face arrière
            5, 4, 7,  5, 7, 6,

            // Face gauche
            8, 9, 10,  8, 10, 11,

            // Face droite
            12, 13, 14,  12, 14, 15,

            // Face supérieure
            16, 17, 18,  16, 18, 19,

            // Face inférieure
            20, 21, 22,  20, 22, 23
        };

    
        GLfloat normals[] = {
            // Face avant
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,

            // Face arrière
            0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, -1.0f,

            // Face gauche
            -1.0f, 0.0f, 0.0f,
            -1.0f, 0.0f, 0.0f,
            -1.0f, 0.0f, 0.0f,
            -1.0f, 0.0f, 0.0f,

            // Face droite
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,

            // Face supérieure
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,

            // Face inférieure
            0.0f, -1.0f, 0.0f,
            0.0f, -1.0f, 0.0f,
            0.0f, -1.0f, 0.0f,
            0.0f, -1.0f, 0.0f
        };

        
        std::vector<GLfloat> vertices;
        for (int i = 0; i < 24; ++i) {
            // Ajouter les positions
            vertices.push_back(positions[i * 3]);
            vertices.push_back(positions[i * 3 + 1]);
            vertices.push_back(positions[i * 3 + 2]);

            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);

            // Ajouter les normales
            vertices.push_back(normals[i * 3]);
            vertices.push_back(normals[i * 3 + 1]);
            vertices.push_back(normals[i * 3 + 2]);
        }

        // Création du VAO
        glGenVertexArrays(1, &m_VAO_id);
        glBindVertexArray(m_VAO_id);

        // Création du VBO pour les positions, normales et couleurs
        glGenBuffers(1, &m_VBO_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO_id);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

        // Création de l'EBO pour les indices
        glGenBuffers(1, &m_EBO_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Configuration des attributs de vertex
        // Positions (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        // Normales (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // Couleurs (location = 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        // Désactiver le VAO
        glBindVertexArray(0);
    }

    ~Boite()
    {
        glDeleteBuffers(1, &m_VBO_id);
        glDeleteBuffers(1, &m_EBO_id);
        glDeleteVertexArrays(1, &m_VAO_id);
    }

    void draw()
    {
        glBindVertexArray(m_VAO_id);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};



//------------------------ S H A D E R   P R O G R A M S ----------------------


class ShaderProg {

public:
    enum ShaderType  { T_VERTEX, T_FRAGMENT, T_GEOMETRY, T_NUM };

    static const char* get_shader_type_name (ShaderType type)
    {
        switch (type) {
            case T_VERTEX   : return "vertex";
            case T_FRAGMENT : return "fragment";
            case T_GEOMETRY : return "geometry";
            default : return "";
        }
    }

    static GLenum get_gl_shader_type (ShaderType type)
    {
        switch (type) {
            case T_VERTEX   : return GL_VERTEX_SHADER;
            case T_FRAGMENT : return GL_FRAGMENT_SHADER;
            case T_GEOMETRY : return GL_GEOMETRY_SHADER;
            default : return 0;
        }
    }

    static ShaderType get_shader_type_from_argv (const char* arg)
    {
        if (!strcmp (arg, "-vs")) return T_VERTEX;
        if (!strcmp (arg, "-fs")) return T_FRAGMENT;
        if (!strcmp (arg, "-gs")) return T_GEOMETRY;
        return T_NUM;
    }


    enum ShaderCateg { C_COLOR, C_TEXTURE, C_DIFFUSE, C_SPECULAR, C_NUM };

    static const char* get_shader_categ_name (ShaderCateg categ)
    {
        switch (categ) {
            case C_COLOR    : return "color";
            case C_TEXTURE  : return "texture";
            case C_DIFFUSE  : return "diffuse";
            case C_SPECULAR : return "specular";
            default : return "";
        }
    }

    static ShaderCateg get_shader_categ_from_name (const char* name)
    {
        if (!strcmp (name, "color")) return C_COLOR;
        if (!strcmp (name, "texture")) return C_TEXTURE;
        if (!strcmp (name, "diffuse")) return C_DIFFUSE;
        if (!strcmp (name, "specular")) return C_SPECULAR;
        return C_NUM;
    }

    static std::string get_usage_for_shader_categs()
    {
        std::string s;
        for (auto categ = C_COLOR; categ < C_NUM; 
                  categ = static_cast<ShaderCateg>((int)categ+1))
        {
            const char* name = get_shader_categ_name (categ);
            if (!name) continue;
            if (s.length() > 0) s+= "|";
            s += name;
        }
        return s;
    }


    const char* m_default_shader_texts[C_NUM][T_NUM] = {

        // C_COLOR : transmet position et couleur
        {
            // Vertex shader
            "#version 330\n"
            "in vec4 vPos;\n"
            "in vec4 vCol;\n"
            "out VertexData {\n"
            "    vec4 color;\n"
            "} vd_out;\n"
            "layout (std140) uniform Uniforms {\n"
            "    mat4 matProj;\n"
            "    mat4 matCam;\n"
            "    vec4 mousePos;\n"
            "    float time;\n"
            "};\n"
            "uniform mat4 matWorld;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    gl_Position = matProj * matCam * matWorld * vPos;\n"
            "    vd_out.color = vCol;\n"
            "}\n",

            // Fragment shader
            "#version 330\n"
            "in VertexData {\n"
            "    vec4 color;\n"
            "} vd_in;\n"
            "out vec4 fragColor;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    fragColor = vd_in.color;\n"
            "}\n",

            // Geometry shader
            ""
        },

        // C_TEXTURE : texture non éclairée
        {
            // Vertex shader
            "#version 330\n"
            "in vec4 vPos;\n"
            "in vec2 vTex;\n"
            "out VertexData {\n"
            "    vec2 texCoord;\n"
            "} vd_out;\n"
            "layout (std140) uniform Uniforms {\n"
            "    mat4 matProj;\n"
            "    mat4 matCam;\n"
            "    vec4 mousePos;\n"
            "    float time;\n"
            "};\n"
            "uniform mat4 matWorld;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    gl_Position = matProj * matCam * matWorld * vPos;\n"
            "    vd_out.texCoord = vTex;\n"
            "}\n",

            // Fragment shader
            "#version 330\n"
            "in VertexData {\n"
            "    vec2 texCoord;\n"
            "} vd_in;\n"
            "out vec4 fragColor;\n"
            "uniform sampler2D uTex;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    fragColor = texture2D (uTex, vd_in.texCoord);\n"
            "}\n",

            // Geometry shader
            ""
        },

        // C_DIFFUSE : normales, lumière ambiante et diffuse
        {
            // Vertex shader
            "#version 330\n"
            "in vec4 vPos;\n"
            "in vec4 vCol;\n"
            "in vec3 vNor;\n"
            "out VertexData {\n"
            "    vec4 color;\n"
            "    vec3 normal;\n"
            "} vd_out;\n"
            "layout (std140) uniform Uniforms {\n"
            "    mat4 matProj;\n"
            "    mat4 matCam;\n"
            "    vec4 mousePos;\n"
            "    float time;\n"
            "};\n"
            "uniform mat4 matWorld;\n"
            "uniform mat3 matNor;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    gl_Position = matProj * matCam * matWorld * vPos;\n"
            "    vd_out.color = vCol;\n"
            "    vd_out.normal = matNor * vNor;\n"
            "}\n",

            // Fragment shader
            "#version 330\n"
            "in VertexData {\n"
            "    vec4 color;\n"
            "    vec3 normal;\n"
            "} vd_in;\n"
            "out vec4 fragColor;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    // Couleur et direction de lumière, normalisée\n"
            "    vec3 lightColor = vec3(0.8);\n"
            "    vec3 lightDir = normalize(vec3(0.0, 0.0, 10.0));\n"
            "\n"
            "    // Normale du fragment, normalisée\n"
            "    vec3 nor3 = normalize(vd_in.normal);\n"
            "\n"
            "    // Cosinus de l'angle entre la normale et la lumière\n"
            "    float cosTheta = dot(nor3, lightDir);\n"
            "\n"
            "    // Lumière diffuse\n"
            "    vec3 diffuse = lightColor * max(cosTheta, 0.0);\n"
            "\n"
            "    // Lumière ambiante\n"
            "    vec3 ambiant = vec3(0.3);\n"
            "\n"
            "    // Somme des lumières\n"
            "    vec3 sumLight = diffuse + ambiant;\n"
            "\n"
            "    // Couleur de l'objet éclairé\n"
            "    vec4 result = vec4(sumLight, 1.0) * vd_in.color;\n"
            "\n"
            "    fragColor = clamp(result, 0.0, 1.0);\n"
            "}\n",

            // Geometry shader
            ""
        },

        // C_SPECULAR : normales, lumière ambiante, diffuse et spéculaire
        {
            // Vertex shader
            "#version 330\n"
            "in vec4 vPos;\n"
            "in vec4 vCol;\n"
            "in vec3 vNor;\n"
            "out VertexData {\n"
            "    vec4 color;\n"
            "    vec4 posit;\n"
            "    vec3 normal;\n"
            "} vd_out;\n"
            "layout (std140) uniform Uniforms {\n"
            "    mat4 matProj;\n"
            "    mat4 matCam;\n"
            "    vec4 mousePos;\n"
            "    float time;\n"
            "};\n"
            "uniform mat4 matWorld;\n"
            "uniform mat3 matNor;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    gl_Position = matProj * matCam * matWorld * vPos;\n"
            "    vd_out.color = vCol;\n"
            "    vd_out.posit = matWorld * vPos;\n"
            "    vd_out.normal = matNor * vNor;\n"
            "}\n",

            // Fragment shader
            "#version 330\n"
            "in VertexData {\n"
            "    vec4 color;\n"
            "    vec4 posit;\n"
            "    vec3 normal;\n"
            "} vd_in;\n"
            "out vec4 fragColor;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    // Couleur et direction de lumière, normalisée\n"
            "    vec3 lightColor = vec3(0.6);\n"
            "    vec3 lightDir = normalize(vec3(0.0, -1.0, 10.0));\n"
            "\n"
            "    // Normale du fragment, normalisée\n"
            "    vec3 nor3 = normalize(vd_in.normal);\n"
            "\n"
            "    // Cosinus de l'angle entre la normale et la lumière\n"
            "    float cosTheta = dot(nor3, lightDir);\n"
            "\n"
            "    // Lumière diffuse\n"
            "    vec3 diffuse = lightColor * max(cosTheta, 0.0);\n"
            "\n"
            "    // Lumière ambiante\n"
            "    vec3 ambiant = vec3(0.2);\n"
            "\n"
            "    // Lumière spéculaire\n"
            "    vec3 eyePos = vec3(0.0, 0.0, 1.0);\n"
            "    vec3 specColor = vec3(1.0, 1.0, 1.0);\n"
            "    float spec_S = 0.2;\n"
            "    float spec_m = 32.0;\n"
            "    vec3 specular = vec3(0);\n"
            "\n"
            "    if (cosTheta > 0.0) {\n"
            "        vec3 R = reflect (-lightDir, nor3);\n"
            "        vec3 V = eyePos - vec3(vd_in.posit);\n"
            "        float cosGamma = dot(normalize(R), normalize(V));\n"
            "        specular = specColor * spec_S * pow(max(cosGamma, 0.0), spec_m);\n"
            "    }\n"
            "\n"
            "    // Somme des lumières\n"
            "    vec3 sumLight = diffuse + specular + ambiant;\n"
            "\n"
            "    // Couleur de l'objet éclairé\n"
            "    vec4 result = vec4(sumLight, 1.0) * vd_in.color;\n"
            "\n"
            "    fragColor = clamp(result, 0.0, 1.0);\n"
            "}\n",

            // Geometry shader
            ""
        }
    };


private:
    ShaderCateg m_shader_categ;
    std::string m_shader_str[T_NUM];
    std::vector<GLuint> m_shaders;
    GLuint m_program = -1;

public:

    ShaderProg (ShaderCateg categ,
               const std::string shader_paths[T_NUM]) 
        : m_shader_categ {categ}
    {
        if (categ < C_COLOR || C_NUM <= categ) return;

        for (auto type = T_VERTEX; type < T_NUM; 
                  type = static_cast<ShaderType>((int)type+1)) {
            m_shader_str[type] = m_default_shader_texts[categ][type];
        }

        m_program = glCreateProgram();
        bind_attrib_locations();

        for (auto type = T_VERTEX; type < T_NUM; 
                  type = static_cast<ShaderType>((int)type+1))
        {
            const std::string& shader_path = shader_paths[type];
            if (shader_path.length() > 0)
                load_shader_code (type, shader_path);
        }
    }


    ~ShaderProg()
    {
        glDeleteProgram (m_program);
    }

    const GLuint get_program()
    {
        return m_program;
    }

    void use_program()
    {
        glUseProgram (m_program);
    }


    // Appeler après glCreateProgram mais avant glLinkProgram
    void bind_attrib_locations()
    {
        for (auto loc = VPOS_LOC; loc < LAST_LOC;
            loc = static_cast<VA_Locations>((int)loc+1))
        {
            const char* name = get_vertex_attribute_name (loc);
            if (!name) continue;
            glBindAttribLocation (m_program, loc, name);
        }
    }


    GLint get_uniform (const char* name)
    {
        return glGetUniformLocation (m_program, name);
    }


    bool compile_program()
    {
        const char* name = get_shader_categ_name(m_shader_categ);
        std::cout << "Begin compilation of " << name << " program...\n";

        for (auto type = T_VERTEX; type < T_NUM; 
                  type = static_cast<ShaderType>((int)type+1))
        {
            std::string& shader_str = m_shader_str[type];
            if (shader_str.length() == 0) continue;

            GLenum gl_shader_type = get_gl_shader_type (type);
            GLuint shader = glCreateShader (gl_shader_type);
            m_shaders.push_back (shader);

            const char* shader_text = shader_str.c_str();
            const char* name = get_shader_type_name (type);
            glShaderSource (shader, 1, &shader_text, NULL);
            if (compile_shader (shader, name))
                glAttachShader (m_program, shader);
        }

        bool ok = link_program();

        // Marque les shaders pour suppression par glDeleteProgram
        for (auto shader : m_shaders) {
            glDeleteShader (shader);
        }

        if (ok) {
            std::cout << "Compilation succeed." << std::endl;
        }
        return ok;
    }


    bool compile_shader (GLuint shader, const char* name)
    {
        std::cout << "Compile " << name << " shader...\n";
        glCompileShader (shader);

        GLint isCompiled = 0;
        glGetShaderiv (shader, GL_COMPILE_STATUS, &isCompiled);
        bool ok = isCompiled == GL_TRUE;

        GLsizei maxLength = 2048, length;
        char infoLog[maxLength];
        glGetShaderInfoLog (shader, maxLength, &length, infoLog);

        if (length > 0) {
            if (isCompiled == GL_TRUE)
                 std::cout << "Compilation messages:\n";
            std::cout << infoLog << std::endl;
        }
        return ok;
    }


    bool link_program()
    {
        std::cout << "Link program...\n";
        glLinkProgram (m_program);
    
        GLint status;
        glGetProgramiv (m_program, GL_LINK_STATUS, &status);
        bool ok = status == GL_TRUE;

        GLsizei maxLength = 2048, length;
        char infoLog[maxLength];
        glGetProgramInfoLog (m_program, maxLength, &length, infoLog);

        if (length > 0) {
            if (status == GL_TRUE)
                 std::cout << "Linking messages:\n";
            else std::cout << "### Linking errors:\n";
            std::cout << infoLog << std::endl;
        }
        return ok;
    }


    bool load_shader_code (const ShaderType type, const std::string path)
    {
        if (type < T_VERTEX || type >= T_NUM) return false;
        if (path == "") return false;

        std::ifstream shader_file;
        std::stringstream shader_stream;

        shader_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try {
            std::cout << "Loading \"" << path << "\" ..." << std::endl;
            shader_file.open (path);
            shader_stream << shader_file.rdbuf();
            shader_file.close();
            m_shader_str[type] = shader_stream.str();
        }
        catch (...) {
            std::cerr << "### Load error: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

    void print_shaders()
    {
        const char* categ_name = get_shader_categ_name(m_shader_categ);

        for (auto type = T_VERTEX; type < T_NUM; 
                  type = static_cast<ShaderType>((int)type+1))
        {
            std::string& shader_str = m_shader_str[type];
            if (shader_str.length() == 0) continue;
            const char* type_name = get_shader_type_name (type);

            std::cout << "\n--------  " << type_name << " shader for " << 
                categ_name << " --------\n\n" << shader_str << std::endl;
        }
    }

}; // ShaderProg

//------------------------------------ A P P ----------------------------------

const double FRAMES_PER_SEC  = 30.0;
const double ANIM_DURATION   = 18.0;

// En salle TP mettre à 0 si l'affichage "bave"
const int NUM_SAMPLES = 16;

enum CamProj { P_ORTHO, P_FRUSTUM, P_MAX };


class MyApp
{
    bool m_ok = false;
    GLFWwindow* m_window = nullptr;
    double m_aspect_ratio = 1.0;
    bool m_anim_flag = false;
    int m_cube_color = 1;
    float m_radius = 0.5;
    float m_anim_angle = 0,  m_start_angle = 0;
    float m_cam_z, m_cam_r, m_cam_near, m_cam_far;
    bool m_depth_flag = true;
    CamProj m_cam_proj;
    WireCube* m_wire_cube_rgb = nullptr;
    WireCube* m_wire_cube_white = nullptr;
    Cylindre* m_cylindre = nullptr;
    Boite* m_boite = nullptr;
    RoueNor* m_roue = nullptr;
    Pedale* m_pedale = nullptr;
    bool m_flag_phong = false;

    std::string m_shader_paths[ShaderProg::C_NUM][ShaderProg::T_NUM];
    std::string m_program_categ_to_print;

    ShaderProg* m_prog_color = nullptr;
    ShaderProg* m_prog_texture = nullptr;
    ShaderProg* m_prog_diffuse = nullptr;
    ShaderProg* m_prog_specular = nullptr;

    vmath::vec4 m_mousePos;  // mouse_x, mouse_y, width, height

    GLuint m_texture_id1, m_texture_id2;
    const char* m_texture_path1 = "side1.png";
    const char* m_texture_path2 = "side2.png";

    GLuint m_UBO_id;


    void load_programs()
    {
        m_prog_color = new ShaderProg {
            ShaderProg::C_COLOR, m_shader_paths[ShaderProg::C_COLOR] };
        m_prog_color->compile_program();

        m_prog_texture = new ShaderProg {
            ShaderProg::C_TEXTURE, m_shader_paths[ShaderProg::C_TEXTURE] };
        m_prog_texture->compile_program();

        m_prog_diffuse = new ShaderProg {
            ShaderProg::C_DIFFUSE, m_shader_paths[ShaderProg::C_DIFFUSE] };
        m_prog_diffuse->compile_program();

        m_prog_specular = new ShaderProg {
            ShaderProg::C_SPECULAR, m_shader_paths[ShaderProg::C_SPECULAR] };
        m_prog_specular->compile_program();
    }


    void tear_programs()
    {
        delete m_prog_color;    m_prog_color = nullptr;
        delete m_prog_texture;  m_prog_texture = nullptr;
        delete m_prog_diffuse;  m_prog_diffuse = nullptr;
        delete m_prog_specular; m_prog_specular = nullptr;
    }


    void print_program_shaders (std::string& categ)
    {
        if (categ.length() > 0) {
            if (categ == "color")
                m_prog_color->print_shaders();
            else if (categ == "texture")
                m_prog_texture->print_shaders();
            else if (categ == "diffuse")
                m_prog_diffuse->print_shaders();
            else if (categ == "specular")
                m_prog_specular->print_shaders();
            else
                std::cerr << "### Error: program " << categ 
                    << " unknown" << std::endl;
        }
    }


    void initGL()
    {
        std::cout << __func__ << std::endl;

        glEnable (GL_DEPTH_TEST);

        load_programs();
        print_program_shaders (m_program_categ_to_print);

        // Init position de la souris au milieu de la fenêtre
        int width, height;
        glfwGetWindowSize (m_window, &width, &height);
        m_mousePos = {width/2.0f, height/2.0f, (float) width, (float) height};

        // Création des textures
        m_texture_id1 = load_texture (m_texture_path1);
        m_texture_id2 = load_texture (m_texture_path2);

        // Création des objets graphiques
        m_wire_cube_white = new WireCube {true,  0.3};
        m_wire_cube_rgb   = new WireCube {false, 0.3};
        m_roue = new RoueNor {VPOS_LOC, VCOL_LOC, VNOR_LOC, 10, 0.2, 0.4, 0.1, 1.0, 0, 0, 0.2};
        m_boite = new Boite{0.5f, 0.8f, 0.5f};
        m_cylindre = new Cylindre{0.3f, 0.5, 36, 1.0f, 0.0f, 0.0f, VPOS_LOC, VCOL_LOC};
        m_pedale = new Pedale{0.5f, 0.7f, 0.3f, 0.03, 0.0f, 0.0, 1.0, VPOS_LOC, VCOL_LOC};

        // Création UBO avec taille réservée
        glGenBuffers (1, &m_UBO_id);
        glBindBuffer (GL_UNIFORM_BUFFER, m_UBO_id);
        glBufferData (GL_UNIFORM_BUFFER, sizeof(UBO_Uniforms), NULL, GL_STATIC_DRAW); 
        glBindBuffer (GL_UNIFORM_BUFFER, 0);

        // On relie le UBO et chaque shader au binding point
        glBindBufferBase (GL_UNIFORM_BUFFER, UBO_BINDING_POINT, m_UBO_id); 
        std::vector<GLuint> program_ids {
             m_prog_color->get_program(),
             m_prog_texture->get_program(),
             m_prog_diffuse->get_program(),
             m_prog_specular->get_program()
        };
        for (GLuint prog : program_ids) {
            glUniformBlockBinding (prog, glGetUniformBlockIndex (prog, "Uniforms"), 
                UBO_BINDING_POINT);
        }
    }


    void tearGL()
    {
        // Destruction des objets graphiques
        delete m_wire_cube_white;
        delete m_wire_cube_rgb;
        glDeleteBuffers (1, &m_UBO_id);
        tear_programs();
    }


    void animate()
    {
        auto frac_part = [](double x){ return x - std::floor(x); };

        // Change la coordonnée en fonction du temps
        double time = glfwGetTime();           // durée depuis init
        double slice = time / ANIM_DURATION;
        double a = frac_part(slice);
        m_anim_angle = m_start_angle + a*360.0;
    }


    void displayGL()
    {
        //glClearColor (0.95, 1.0, 0.8, 1.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vmath::mat4 mat_proj, mat_cam, mat_world, mat_MVP;
        vmath::mat3 mat_Nor;
        set_projection (mat_proj, mat_cam);

        // On met les données dans le UBO
        glBindBuffer (GL_UNIFORM_BUFFER, m_UBO_id);
        glBufferSubData (GL_UNIFORM_BUFFER, 
            offsetof(UBO_Uniforms, matProj), sizeof(vmath::mat4), &mat_proj);
        glBufferSubData (GL_UNIFORM_BUFFER, 
            offsetof(UBO_Uniforms, matCam), sizeof(vmath::mat4), &mat_cam);
        glBufferSubData (GL_UNIFORM_BUFFER, 
            offsetof(UBO_Uniforms, mousePos), sizeof(vmath::vec4), &m_mousePos);
        GLfloat time_f = glfwGetTime();  // GLdouble nécessite v4.0+, mal géré
        glBufferSubData (GL_UNIFORM_BUFFER, 
            offsetof(UBO_Uniforms, time), sizeof(GLfloat), &time_f);
        glBindBuffer (GL_UNIFORM_BUFFER, 0);

        ShaderProg* prog = nullptr;



        // -------- Centre -----------

        prog = m_prog_color;
        prog->use_program();
        
        mat_world = vmath::translate (0.f, 0.f, 0.f) * vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f);;
        glUniformMatrix4fv (prog->get_uniform ("matWorld"), 1, GL_FALSE, mat_world);

        if (m_cube_color == 1)
            m_wire_cube_white->draw();
        else if (m_cube_color == 2)
            m_wire_cube_rgb->draw();

        //-------- En haut à gauche --------

        prog = m_prog_color;
        prog->use_program();

        mat_world = vmath::translate (-0.8f, +0.7f, 0.f)
                    * vmath::scale (0.7f)
                    * vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f);

        glUniformMatrix4fv (prog->get_uniform ("matWorld"), 1, GL_FALSE, mat_world);

        m_cylindre->draw();

        //-------- En haut à droite --------

        prog = m_prog_color;
        prog->use_program();

        mat_world = vmath::translate (0.8f, +0.7f, 0.f)
                    * vmath::scale (0.7f)
                    * vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f);

        glUniformMatrix4fv (prog->get_uniform ("matWorld"), 1, GL_FALSE, mat_world);

        m_pedale->draw ();

        //-------- En bas à gauche --------

        prog = m_prog_diffuse;
        prog->use_program();

        mat_world = vmath::translate (-0.8f, -0.7f, 0.f)
                    * vmath::scale (0.9f)
                    * vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f)
                    * vmath::rotate (-20.0f, 1.f, 0.f, 0.f);
        mat_Nor = vmath::normal (mat_world);

        glUniformMatrix4fv (prog->get_uniform ("matWorld"), 1, GL_FALSE, mat_world);
        glUniformMatrix3fv (prog->get_uniform ("matNor"), 1, GL_FALSE, mat_Nor);

        m_boite->draw();

        //-------- En bas à droite --------

        prog = m_prog_specular;
        prog->use_program();

        mat_world = vmath::translate (0.8f, -0.7f, 0.f)
                    * vmath::scale (0.9f)
                    * vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f)
                    * vmath::rotate (-20.0f, 1.f, 0.f, 0.f);
        mat_Nor = vmath::normal (mat_world);

        glUniformMatrix4fv (prog->get_uniform ("matWorld"), 1, GL_FALSE, mat_world);
        glUniformMatrix3fv (prog->get_uniform ("matNor"), 1, GL_FALSE, mat_Nor);

        m_roue->draw();

    }


    void set_projection (vmath::mat4& mat_proj, vmath::mat4& mat_cam)
    {
        mat_proj = vmath::mat4::identity();

        GLfloat hr = m_cam_r, wr = hr * m_aspect_ratio;
        switch (m_cam_proj) {
        case P_ORTHO :
            mat_proj = vmath::ortho (-wr, wr, -hr, hr, m_cam_near, m_cam_far);
            break;
        case P_FRUSTUM :
            mat_proj = vmath::frustum (-wr, wr, -hr, hr, m_cam_near, m_cam_far);
            break;
        default: ;
        }

        vmath::vec3 eye {0, 0, (float)m_cam_z}, center {0, 0, 0}, up {0, 1., 0};
        mat_cam = vmath::lookat (eye, center, up);
    }


    GLuint load_texture (const char* path)
    {
        GLuint texture_id;

        glGenTextures (1, &texture_id);
        glBindTexture (GL_TEXTURE_2D, texture_id);

        // Options de filtrage pour le mipmap
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Charge l'image et génère la texture
        int width, height, n_comp;
        std::cout << "Loading texture \"" << path << "\" ..." << std::endl;
        unsigned char *data = stbi_load (path, &width, &height, &n_comp, 0);

        if (!data) {
            std::cout << "### Loading error: " << stbi_failure_reason() << std::endl;
            glDeleteTextures (1, &texture_id);
            return 0;
        }

        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, 
            GL_UNSIGNED_BYTE, data);
        glGenerateMipmap (GL_TEXTURE_2D);
        stbi_image_free (data);

        return texture_id;
    }


    void cam_init()
    {
        m_cam_z = 3; m_cam_r = 0.5; m_cam_near = 1; m_cam_far = 5; 
        m_cam_proj = P_FRUSTUM;
    }


    void set_viewport (int width, int height)
    {
        glViewport (0, 0, width, height);
        m_aspect_ratio = (double) width / height;
    }


    static void on_reshape_func (GLFWwindow* window, int width, int height)
    {
        std::cout << __func__ << " "
            << width << " " << height << std::endl;

        MyApp* that = static_cast<MyApp*>(glfwGetWindowUserPointer (window));
        that->set_viewport (width, height);
    }


    static void print_help()
    {
        std::cout << "h help  i init  a anim  p proj  zZ cam_z  rR radius  nN near  "
                  << "fF far  dD dist  b z-buffer  c cube  u update program  o Phong"
                  << std::endl;
    }


    void print_projection()
    {
        switch (m_cam_proj) {
        case P_ORTHO   : std::cout << "Ortho:   ";   break;
        case P_FRUSTUM : std::cout << "Frustum: "; break;
        default: ;
        }
        GLfloat hr = m_cam_r, wr = hr * m_aspect_ratio;
        std::cout << std::fixed << std::setprecision(1) 
                  << -wr  << ", " <<  wr  << ", " 
                  << -hr  << ", " <<  hr  << ", " 
                  << m_cam_near  << ", " <<  m_cam_far << " ; " 
                  << "cam_z = " << m_cam_z << std::endl;
    }


    static void on_mouse_func (GLFWwindow* window, double xpos, double ypos)
    {
        //std::cout << __func__ << " " << xpos << " " << ypos << std::endl;

        MyApp* that = static_cast<MyApp*>(glfwGetWindowUserPointer (window));
        if (!that->m_ok) return;

        int width, height;
        glfwGetWindowSize (window, &width, &height);
        that->m_mousePos[0] = xpos;
        that->m_mousePos[1] = height - ypos;  // origine en bas à gauche
        that->m_mousePos[2] = width;
        that->m_mousePos[3] = height;
    }


    static void on_key_func (GLFWwindow* window, int key, int scancode, 
        int action, int mods)
    {
        //std::cout << __func__ << " " << key << " " << scancode << " " 
        //    << action << " " << mods << std::endl;

        // action = GLFW_PRESS ou GLFW_REPEAT ou GLFW_RELEASE
        if (action == GLFW_RELEASE) return;

        MyApp* that = static_cast<MyApp*>(glfwGetWindowUserPointer (window));

        int trans_key = translate_qwerty_to_azerty (key, scancode);
        switch (trans_key) {

        case GLFW_KEY_I :
            that->cam_init();
            break;
        case GLFW_KEY_A :
            that->m_anim_flag = !that->m_anim_flag;
            if (that->m_anim_flag) {
                that->m_start_angle = that->m_anim_angle;
                glfwSetTime (0);
            }
            break;
        case GLFW_KEY_P : {
            int k = static_cast<int>(that->m_cam_proj) + 1;
            if (k >= static_cast<int>(P_MAX)) k = 0;
            that->m_cam_proj = static_cast<CamProj>(k);
            // Heuristique pour garder sensiblement la même taille
            if (that->m_cam_proj == P_FRUSTUM) that->m_cam_r /= 2.5;
            else if (that->m_cam_proj == P_ORTHO) that->m_cam_r *= 2.5;
            break;
        }
        case GLFW_KEY_Z :
            change_val_mods (that->m_cam_z, mods, 0.1, -100);
            break;
        case GLFW_KEY_R :
            change_val_mods (that->m_cam_r, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_N :
            change_val_mods (that->m_cam_near, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_F :
            change_val_mods (that->m_cam_far, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_D :
            change_val_mods (that->m_cam_z, mods, 0.1, -100);
            change_val_mods (that->m_cam_near, mods, 0.1, 0.1);
            change_val_mods (that->m_cam_far, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_B :
            that->m_depth_flag = !that->m_depth_flag;
            std::cout << "depth_flag is " << that->m_depth_flag << std::endl;
            if (that->m_depth_flag) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
            break;
        case GLFW_KEY_C :
            that->m_cube_color = (that->m_cube_color+1) % 3;
            break;
        case GLFW_KEY_U :
            that->tear_programs();
            that->load_programs();
            break;
        case GLFW_KEY_O :
            that->m_flag_phong = !that->m_flag_phong;
            break;
        case GLFW_KEY_H :
            print_help();
            break;
        case GLFW_KEY_ESCAPE :
            that->m_ok = false;
            break;
        default: 
            return;
        }

        that->print_projection();
    }


    template <typename T>
    static void change_val_mods (T& val, int mods, double incr, double min_val)
    {
        val += (mods & GLFW_MOD_SHIFT) ? incr : -incr;
        if (val <= min_val) val = min_val;
    }


    static int translate_qwerty_to_azerty (int key, int scancode)
    {
        // https://www.glfw.org/docs/latest/group__keys.html
        // QWERTY -> AZERTY
        switch (key) {
            case GLFW_KEY_Q : return GLFW_KEY_A;
            case GLFW_KEY_A : return GLFW_KEY_Q;
            case GLFW_KEY_W : return GLFW_KEY_Z;
            case GLFW_KEY_Z : return GLFW_KEY_W;
            case GLFW_KEY_SEMICOLON : return GLFW_KEY_M;
        }

        // Détection des différences non corrigées
        const char* name = glfwGetKeyName (key, scancode);
        if (name != NULL) { 
            int capital = toupper(name[0]);
            if (capital != key) {
                std::cout << __func__ << " DIFF " 
                    << capital << " " << key << std::endl;
            }
        }
        return key;
    }


    static void on_error_func (int error, const char* description)
    {
        std::cerr << "### Error: " << description << std::endl;
    }


    bool parse_args (int argc, char* argv[])
    {
        int i = 1;
        while (i < argc) {

            auto type = ShaderProg::get_shader_type_from_argv (argv[i]);
            if (type != ShaderProg::T_NUM && i+1 < argc) {
                auto categ = ShaderProg::get_shader_categ_from_name (argv[i+1]);
                if (categ != ShaderProg::C_NUM && i+2 < argc) {
                    m_shader_paths[categ][type] = argv[i+2];
                    i += 3; continue;
                }
            }
            if (strcmp(argv[i], "-ps") == 0 && i+1 < argc) {
                m_program_categ_to_print = argv[i+1];
                i += 2 ; continue;
            }
            if (strcmp(argv[i], "--help") == 0) {
                std::cout << "USAGE:\n"
                    << "  " << argv[0] << " [-vs|-fs|-gs categ path] [-ps categ]\n"
                    << "  categ: " << ShaderProg::get_usage_for_shader_categs()
                    << std::endl;
                return false;
            }
            std::cerr << "### Error, bad arguments. Try --help" << std::endl;
            return false;
        }
        return true;
    }


public:

    MyApp (int argc, char* argv[])
    {
        if (!parse_args (argc, argv)) return;

        if (!glfwInit()) {
            std::cerr << "GLFW: initialization failed" << std::endl;
            return;
        }
        glfwSetErrorCallback (on_error_func);

        // Hints à spécifier avant la création de la fenêtre
        //   https://www.glfw.org/docs/latest/window.html#window_hints_fb

        if (NUM_SAMPLES > 0)
            glfwWindowHint (GLFW_SAMPLES, NUM_SAMPLES);

        // On demande une version spécifique d'OpenGL
        glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
        //glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Création de la fenêtre
        m_window = glfwCreateWindow (640, 480, "UBO", NULL, NULL);
        if (!m_window) {
            std::cerr << "GLFW: window creation failed" << std::endl;
            return;
        }

        // Les callbacks pour GLFW étant statiques, on mémorise l'instance
        glfwSetWindowUserPointer (m_window, this);
        glfwSetWindowSizeCallback (m_window, on_reshape_func);
        glfwSetCursorPosCallback (m_window, on_mouse_func);
        glfwSetKeyCallback (m_window, on_key_func);

        // Rend le contexte GL courant. Tous les appels GL seront placés après.
        glfwMakeContextCurrent (m_window);
        glfwSwapInterval (1);
        m_ok = true;

        cam_init();
        print_help();

        // Initialisation de la machinerie GL en utilisant GLAD.
        gladLoadGL(); 
        std::cout << "Loaded OpenGL "
            << GLVersion.major << "." << GLVersion.minor << std::endl;

        // Mise à jour viewport et ratio avec taille réelle de la fenêtre
        int width, height;
        glfwGetWindowSize (m_window, &width, &height);
        set_viewport (width, height);

        initGL();
    }


    void run()
    {
        while (m_ok && !glfwWindowShouldClose (m_window))
        {
            displayGL();
            glfwSwapBuffers (m_window);

            if (m_anim_flag) {
                glfwWaitEventsTimeout (1.0/FRAMES_PER_SEC);
                animate();
            }
            else glfwWaitEvents();
        }
    }

    ~MyApp()
    {
        if (m_ok) tearGL();
        if (m_window) glfwDestroyWindow (m_window);
        glfwTerminate();
    }

}; // MyApp


int main(int argc, char* argv[]) 
{
    MyApp app {argc, argv};
    app.run();
}

