/*
    Rendu de lumières

    CC BY-SA Edouard.Thiel@univ-amu.fr - 30/01/2025
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
//--------------------------------- K I T E -----------------------------------

class Kite
{
    GLuint m_VAO_id, m_VBO_id;
    GLint m_vPos_loc, m_vCol_loc, m_vNor_loc;

public:
    Kite (GLint vPos_loc, GLint vCol_loc, GLint vNor_loc)
        : m_vPos_loc {vPos_loc}, m_vCol_loc {vCol_loc}, m_vNor_loc {vNor_loc}
    {
        // Positions
        GLfloat positions[] = {
            0.2,  0.5,  0.2,    // 0 A
            0.1, -0.6,  0.1,    // 1 B
           -0.8,  0.1, -0.3,    // 2 C
            0.8, -0.1, -0.1,    // 3 D
        };
                         // A  C  B  A  B  D
        GLint ind_pos[] = { 0, 2, 1, 0, 1, 3 };

        // Couleurs
        GLfloat colors[] = {
            1.0, 0.6, 0.6,      // triangle 0
            0.7, 1.0, 0.5,      // triangle 1
        };
                       // A  C  B  A  B  D
        int ind_col[] = { 0, 0, 0, 1, 1, 1 };

        // Conversion des positions en vec3
        auto pos_to_v = [&](int i) { 
            return vmath::vec3( positions[i*3], positions[i*3+1], positions[i*3+2]); 
        };
        vmath::vec3 pA = pos_to_v(0), pB = pos_to_v(1),
                    pC = pos_to_v(2), pD = pos_to_v(3);
        vmath::vec3 vAB = pB-pA, vAC = pC-pA, vAD = pD-pA;

        // Calcul des normales
        vmath::vec3 nACB = vmath::cross (vAC, vAB),
                    nABD = vmath::cross (vAB, vAD);
        // Normalisation
        vmath::normalize (nACB);
        vmath::normalize (nABD);

        // Stockage des normales normalisées
        GLfloat normals[] = { nACB[0], nACB[1], nACB[2],
                              nABD[0], nABD[1], nABD[2] };
                       // A  C  B  A  B  D
        int ind_nor[] = { 0, 0, 0, 1, 1, 1 };

        // Création d'une structure de données à plat
        std::vector<GLfloat> vertices;
        for (int i = 0; i < 6; i++) {
            // Positions sommets
            for (int j = 0; j < 3; j++)
                vertices.push_back (positions[ind_pos[i]*3+j]);
            // Couleurs sommets
            for (int j = 0; j < 3; j++)
                vertices.push_back (colors[ind_col[i]*3+j]);
            // Normales sommets
            for (int j = 0; j < 3; j++)
                vertices.push_back (normals[ind_nor[i]*3+j]);
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


    ~Kite()
    {
        glDeleteBuffers (1, &m_VBO_id);
        glDeleteVertexArrays (1, &m_VAO_id);
    }


    void draw ()
    {
        glBindVertexArray (m_VAO_id);

        glDrawArrays (GL_TRIANGLES, 0, 6);

        glBindVertexArray (0);
    }

}; // Kite


// ---- ROUE -----

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

//------------------------------------ A P P ----------------------------------

const double FRAMES_PER_SEC  = 30.0;
const double ANIM_DURATION   = 18.0;

// En salle TP mettre à 0 si l'affichage "bave"
const int NUM_SAMPLES = 16;

enum CamProj { P_ORTHO, P_FRUSTUM, P_MAX };


class MyApp
{
    bool m_ok = false;
    float m_angle = 0.0f;
    GLFWwindow* m_window = nullptr;
    double m_aspect_ratio = 1.0;
    bool m_anim_flag = false;
    int m_cube_color = 1;
    float m_radius = 0.5;
    float m_anim_angle = 0,  m_start_angle = 0;
    float m_cam_z, m_cam_r, m_cam_near, m_cam_far;
    bool m_depth_flag = true;
    CamProj m_cam_proj;
    Kite* m_kite = nullptr;
    RoueNor* m_roue = nullptr;

    const char* m_default_vertex_shader_text =
        "#version 330\n"
        "in vec4 vPos;\n"
        "in vec4 vCol;\n"
        "in vec3 vNor;\n"
        "out vec4 color;\n"
        "out vec3 trNor;\n"
        "uniform mat4 matMVP;\n"
        "uniform mat3 matNor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = matMVP * vPos;\n"
        "    color = vCol;\n"
        "    trNor = matNor * vNor;\n"
        "}\n";

    const char* m_default_fragment_shader_text =
        "#version 330\n"
        "in vec4 color;\n"
        "in vec3 trNor;\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    // Couleur et direction de lumière, normalisée\n"
        "    vec3 lightColor = vec3(0.8);\n"
        "    vec3 lightDir = normalize(vec3(0.0, 0.0, 10.0));\n"
        "\n"
        "    // Normale du fragment, normalisée\n"
        "    vec3 nor3 = normalize(trNor);\n"
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
        "    vec4 result = vec4(sumLight, 1.0) * color;\n"
        "\n"
        "    fragColor = clamp(result, 0.0, 1.0);\n"
        "}\n";

    std::string m_vertex_shader_path, m_fragment_shader_path;

    GLuint m_program = 0;
    GLint m_vPos_loc, m_vCol_loc, m_vNor_loc;
    GLint m_matMVP_loc, m_matNor_loc;

    vmath::vec4 m_mousePos;  // mouse_x, mouse_y, width, height
    GLint m_mousePos_loc;


    void animate()
    {
        auto frac_part = [](double x){ return x - std::floor(x); };

        // Change la coordonnée en fonction du temps
        double time = glfwGetTime();           // durée depuis init
        double slice = time / ANIM_DURATION;
        double a = frac_part(slice);
        m_anim_angle = m_start_angle + a*360.0;
    }


    void initGL()
    {
        std::cout << __func__ << std::endl;

        glEnable (GL_DEPTH_TEST);

        m_program = load_and_compile_program (m_vertex_shader_path, 
            m_fragment_shader_path);

        // Récupère l'identifiant des "variables" dans les shaders
        m_vPos_loc = glGetAttribLocation (m_program, "vPos");
        m_vCol_loc = glGetAttribLocation (m_program, "vCol");
        m_vNor_loc = glGetAttribLocation (m_program, "vNor");
        m_matMVP_loc = glGetUniformLocation (m_program, "matMVP");
        m_matNor_loc = glGetUniformLocation (m_program, "matNor");
        m_mousePos_loc = glGetUniformLocation (m_program, "mousePos");

        // Init position de la souris au milieu de la fenêtre
        int width, height;
        glfwGetWindowSize (m_window, &width, &height);
        m_mousePos = {width/2.0f, height/2.0f, (float) width, (float) height};

        // Création des objets graphiques
        m_kite = new Kite {m_vPos_loc, m_vCol_loc, m_vNor_loc};
        m_roue = new RoueNor {m_vPos_loc, m_vCol_loc, m_vNor_loc, 10, 0.5, 1.0, 0.2, 1.0, 0, 0, 0.2};
    }


    void tearGL()
    {
        // Destruction des objets graphiques
        delete m_kite;

        glDeleteProgram (m_program);
    }


    void displayGL()
    {
        //glClearColor (0.95, 1.0, 0.8, 1.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram (m_program);

        vmath::mat4 mat_MVP;
        vmath::mat3 mat_Nor;
        set_projection (mat_MVP, mat_Nor);
        glUniformMatrix4fv (m_matMVP_loc, 1, GL_FALSE, mat_MVP);
        glUniformMatrix3fv (m_matNor_loc, 1, GL_FALSE, mat_Nor);

        // Position souris si mousePos présente dans le shader program
        if (m_mousePos_loc != -1)
            glUniform4fv (m_mousePos_loc, 1, m_mousePos);

        // Dessins

        // vmath::mat4 translatedMatrix1 = mat_MVP;
        // translatedMatrix1 = translatedMatrix1 * vmath::rotate(static_cast<float>(m_angle * 180.0 / M_PI), 0.f, 0.0f, 1.f);
        // glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix1);
        
        m_roue->draw ();

    }


    void set_projection (vmath::mat4& mat_MVP, vmath::mat3& mat_Nor)
    {
        vmath::mat4 mat_proj = vmath::mat4::identity();

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
        vmath::mat4 mat_cam = vmath::lookat (eye, center, up);

        // Rotation de la scène pour l'animation, tout en float pour le template
        vmath::mat4 mat_world = vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f);

        // Matrice Model Vue Projection
        mat_MVP = mat_proj * mat_cam * mat_world;

        // Matrice normale = transposée de l'inverse du mineur
        mat_Nor = vmath::normal (mat_world);
    }


    void compile_shader (GLuint shader, const char* name)
    {
        std::cout << "Compile " << name << " shader...\n";
        glCompileShader (shader);

        GLint isCompiled = 0;
        glGetShaderiv (shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) m_ok = false;

        GLsizei maxLength = 2048, length;
        char infoLog[maxLength];
        glGetShaderInfoLog (shader, maxLength, &length, infoLog);

        if (length == 0) return;
        if (isCompiled == GL_TRUE)
             std::cout << "Compilation messages:\n";
        else std::cout << "### Compilation errors:\n";
        std::cout << infoLog << std::endl;
    }


    void link_program (GLuint program)
    {
        std::cout << "Link program...\n";
        glLinkProgram (program);
    
        GLint status;
        glGetProgramiv (program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) m_ok = false;

        GLsizei maxLength = 2048, length;
        char infoLog[maxLength];
        glGetProgramInfoLog (program, maxLength, &length, infoLog);

        if (length == 0) return;
        if (status == GL_TRUE)
             std::cout << "Linking messages:\n";
        else std::cout << "### Linking errors:\n";
        std::cout << infoLog << std::endl;
    }


    std::string load_shader_code (const std::string path, const char* default_text)
    {
        if (path == "") return std::string(default_text);

        std::string shader_code;
        std::ifstream shader_file;
        std::stringstream shader_stream;

        shader_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try {
            std::cout << "Loading \"" << path << "\" ..." << std::endl;
            shader_file.open (path);
            shader_stream << shader_file.rdbuf();
            shader_file.close();
            shader_code = shader_stream.str();
        }
        catch (...) {
            std::cerr << "### Load error: " << strerror(errno) << std::endl;
            return std::string(default_text);
        }

        return shader_code;
    }


    GLuint load_and_compile_program (const std::string vertex_shader_path, 
                                     const std::string fragment_shader_path)
    {
        std::string vertex_shader_code = load_shader_code (vertex_shader_path,
            m_default_vertex_shader_text);
        std::string fragment_shader_code = load_shader_code (fragment_shader_path,
            m_default_fragment_shader_text);

        const char* vertex_shader_text = vertex_shader_code.c_str();
        const char* fragment_shader_text = fragment_shader_code.c_str();

        const GLuint vertex_shader = glCreateShader (GL_VERTEX_SHADER);
        glShaderSource (vertex_shader, 1, &vertex_shader_text, NULL);
        compile_shader (vertex_shader, "vertex");

        const GLuint fragment_shader = glCreateShader (GL_FRAGMENT_SHADER);
        glShaderSource (fragment_shader, 1, &fragment_shader_text, NULL);
        compile_shader (fragment_shader, "fragment");
     
        GLuint program = glCreateProgram();
        glAttachShader (program, vertex_shader);
        glAttachShader (program, fragment_shader);
        link_program (program);

        // Marque les shaders pour suppression lorsque glDeleteProgram sera appelé
        glDeleteShader (vertex_shader);
        glDeleteShader (fragment_shader);

        return program;
    }


    void reload_program ()
    {
        glDeleteProgram (m_program);
        m_program = load_and_compile_program (m_vertex_shader_path, 
            m_fragment_shader_path);
        m_mousePos_loc = glGetUniformLocation (m_program, "mousePos");
        std::cout << "Uniform mousePos " << 
            ((m_mousePos_loc == -1) ? "not found" : "found") << std::endl;
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
        std::cout << "h help  i init  a anim  p proj  zZ cam_z  rR radius  "
                  << "nN near  fF far  dD dist  b z-buffer  u update program"
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
            that->cam_init ();
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
        case GLFW_KEY_L :
        // Toggle the fill flag
        flag_fill = !flag_fill;
        std::cout << "Flag fill is now " << (flag_fill ? "ON" : "OFF") << std::endl;
        break;
        case GLFW_KEY_C :
            that->m_cube_color = (that->m_cube_color+1) % 3;
            break;
        case GLFW_KEY_U :
            that->reload_program ();
            break;
        case GLFW_KEY_H :
            print_help ();
            break;

        case GLFW_KEY_SPACE :
        that->m_angle+= 0.2f;
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
        std::cerr << "Error: " << description << std::endl;
    }


    bool parse_args (int argc, char* argv[])
    {
        int i = 1;
        while (i < argc) {
            if (strcmp(argv[i], "-vs") == 0 && i+1 < argc) {
                m_vertex_shader_path = argv[i+1]; 
                i += 2; continue;
            }
            if (strcmp(argv[i], "-fs") == 0 && i+1 < argc) {
                m_fragment_shader_path = argv[i+1]; 
                i += 2; continue;
            }
            if (strcmp(argv[i], "--help") == 0) {
                std::cout << "Options: -vs vs_file -fs fs_file -ps\n";
                return false;
            }
            if (strcmp(argv[i], "-ps") == 0) {
                std::cout << "---- Default vertex shader: ----\n\n"
                          << m_default_vertex_shader_text
                          << "\n---- Default fragment shader: ----\n\n"
                          << m_default_fragment_shader_text << "\n\n";
                return false;
            }
            std::cerr << "Error, bad arguments. Try --help" << std::endl;
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
        m_window = glfwCreateWindow (640, 480, "Rendu de lumière", NULL, NULL);
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

