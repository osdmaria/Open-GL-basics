/*
    Lumière ambiente + diffuse + speculaire

    CC BY-SA Edouard.Thiel@univ-amu.fr - 02/02/2025
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
    bool m_phong;

public:
    Kite (GLint vPos_loc, GLint vCol_loc, GLint vNor_loc, bool phong)
        : m_vPos_loc {vPos_loc}, m_vCol_loc {vCol_loc}, m_vNor_loc {vNor_loc},
          m_phong {phong}
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

        std::vector<GLfloat> normals;
        std::vector<int> ind_nor;
        if (m_phong) {
            vmath::vec3 nAB = (nACB + nABD) / 2.0;

            // Stockage des normales normalisées pour lissage de Phong
            normals = { nACB[0], nACB[1], nACB[2],
                        nABD[0], nABD[1], nABD[2],
                        nAB [0], nAB [1], nAB [2] };
                     // A  C  B  A  B  D
            ind_nor = { 2, 0, 2, 2, 2, 1 };
        }
        else {
            // Stockage des normales normalisées
            normals = { nACB[0], nACB[1], nACB[2],
                        nABD[0], nABD[1], nABD[2] };
                     // A  C  B  A  B  D
            ind_nor = { 0, 0, 0, 1, 1, 1 };
        }

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

class Sphere 
{
    GLuint m_VAO_id, m_VBO_id;
    GLint m_vPos_loc, m_vCol_loc, m_vNor_loc;
    float m_rayon = 0.5f;
    int m_nb_etapes;
    double m_coul_r, m_coul_v, m_coul_b; 
    bool m_flag_lissage = false;

public:
    Sphere (GLint vPos_loc, GLint vCol_loc, GLint vNor_loc,double coul_r, double coul_v, double coul_b, bool flag_lissage, int nb_etapes)
    : m_vPos_loc {vPos_loc}, m_vCol_loc {vCol_loc}, m_vNor_loc {vNor_loc}, 
    m_coul_r {coul_r},
    m_coul_v {coul_v},
    m_coul_b {coul_b},
    m_flag_lissage {flag_lissage},
    m_nb_etapes(nb_etapes){

        
        std::vector<GLfloat> vertices;
        std::vector<vmath::vec3> initial_triangles = { // les 8 triangles initiaux de la sphere
            {m_rayon, 0.0f, 0.0f}, {0.0f, m_rayon, 0.0f}, {0.0f, 0.0f, m_rayon},
            {m_rayon, 0.0f, 0.0f}, {0.0f, 0.0f, -m_rayon}, {0.0f, m_rayon, 0.0f},
            {m_rayon, 0.0f, 0.0f}, {0.0f, 0.0f, m_rayon}, {0.0f, -m_rayon, 0.0f},
            {m_rayon, 0.0f, 0.0f}, {0.0f, -m_rayon, 0.0f}, {0.0f, 0.0f, -m_rayon},
            {-m_rayon, 0.0f, 0.0f}, {0.0f, 0.0f, m_rayon}, {0.0f, m_rayon, 0.0f},
            {-m_rayon, 0.0f, 0.0f}, {0.0f, m_rayon, 0.0f}, {0.0f, 0.0f, -m_rayon},
            {-m_rayon, 0.0f, 0.0f}, {0.0f, -m_rayon, 0.0f}, {0.0f, 0.0f, m_rayon},
            {-m_rayon, 0.0f, 0.0f}, {0.0f, 0.0f, -m_rayon}, {0.0f, -m_rayon, 0.0f}
        };

        for (int i = 0; i < initial_triangles.size(); i += 3) {
            decouper_triangle(initial_triangles[i], initial_triangles[i+1], initial_triangles[i+2], 0, vertices);
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

    ~Sphere()
    {
        glDeleteBuffers (1, &m_VBO_id);
        glDeleteVertexArrays (1, &m_VAO_id);
    }

    void draw ()
    {   
        glPolygonMode (GL_FRONT_AND_BACK, flag_fill ? GL_FILL : GL_LINE);
        glBindVertexArray (m_VAO_id);

        glDrawArrays (GL_TRIANGLES, 0, m_nb_etapes * 8 * 3 * (1 << (2 * m_nb_etapes)));

        glBindVertexArray (0);
    }

    private:
    void decouper_triangle(vmath::vec3 A, vmath::vec3 B, vmath::vec3 C, int n, std::vector<GLfloat>& vertices)
    {
        if (n == m_nb_etapes) {
            vmath::vec3 normale;
            if (m_flag_lissage) {
                // Normale = vecteur OP (du centre au point P)
                normale = vmath::normalize(A);
            } else {
                // Normale du triangle (non lissé)
                normale = vmath::normalize(vmath::cross(B - A, C - A));
            }
    
            // Ajouter les positions, couleurs et normales au vecteur vertices
            for (const auto& point : {A, B, C}) {
                vertices.push_back(point[0]);
                vertices.push_back(point[1]);
                vertices.push_back(point[2]);
                vertices.push_back(m_coul_r);
                vertices.push_back(m_coul_v);
                vertices.push_back(m_coul_b);
    
                if (m_flag_lissage) {
                    vmath::vec3 normal_point = vmath::normalize(point);
                    vertices.push_back(normal_point[0]);
                    vertices.push_back(normal_point[1]);
                    vertices.push_back(normal_point[2]);
                } else {
                    vertices.push_back(normale[0]);
                    vertices.push_back(normale[1]);
                    vertices.push_back(normale[2]);
                }
            }
            return;
        }
    
        // Calcul des points D, E et F
        vmath::vec3 D = vmath::normalize(A + B) * m_rayon;  // OA+OB / norm (OA+OB) est équivalent à normalize (A+B)
        vmath::vec3 E = vmath::normalize(B + C) * m_rayon;
        vmath::vec3 F = vmath::normalize(C + A) * m_rayon;
    
        decouper_triangle(A, D, F, n + 1, vertices);
        decouper_triangle(D, B, E, n + 1, vertices); // appels recursifs
        decouper_triangle(E, C, F, n + 1, vertices);
        decouper_triangle(D, E, F, n + 1, vertices);
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
    GLFWwindow* m_window = nullptr;
    double m_aspect_ratio = 1.0;
    bool m_anim_flag = false;
    int m_cube_color = 1;
    float m_radius = 0.5;
    float m_anim_angle = 0,  m_start_angle = 0;
    float m_cam_z, m_cam_r, m_cam_near, m_cam_far;
    bool m_depth_flag = true;
    CamProj m_cam_proj;
    Kite* m_kite1 = nullptr;
    Kite* m_kite2 = nullptr;
    Sphere* m_sphere1 = nullptr;
    Sphere* m_sphere2 = nullptr;
    bool m_flag_phong = true;

    const char* m_default_vertex_shader_text =
        "#version 330\n"
        "in vec4 vPos;\n"
        "in vec4 vCol;\n"
        "in vec3 vNor;\n"
        "out vec4 color;\n"
        "out vec4 trPos;\n"
        "out vec3 trNor;\n"
        "uniform mat4 matMVP;\n"
        "uniform mat4 matWorld;\n"
        "uniform mat3 matNor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = matMVP * vPos;\n"
        "    color = vCol;\n"
        "    trPos = matWorld * vPos;\n"
        "    trNor = matNor * vNor;\n"
        "}\n";

    const char* m_default_fragment_shader_text =
        "#version 330\n"
        "in vec4 color;\n"
        "in vec4 trPos;\n"
        "in vec3 trNor;\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    // Couleur et direction de lumière, normalisée\n"
        "    vec3 lightColor = vec3(0.6);\n"
        "    vec3 lightDir = normalize(vec3(0.0, -1.0, 10.0));\n"
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
        "        vec3 V = eyePos - vec3(trPos);\n"
        "        float cosGamma = dot(normalize(R), normalize(V));\n"
        "        specular = specColor * spec_S * pow(max(cosGamma, 0.0), spec_m);\n"
        "    }\n"
        "\n"
        "    // Somme des lumières\n"
        "    vec3 sumLight = diffuse + specular + ambiant;\n"
        "\n"
        "    // Couleur de l'objet éclairé\n"
        "    vec4 result = vec4(sumLight, 1.0) * color;\n"
        "\n"
        "    fragColor = clamp(result, 0.0, 1.0);\n"
        "}\n";

    std::string m_vertex_shader_path, m_fragment_shader_path;

    GLuint m_program = 0;
    GLint m_vPos_loc, m_vCol_loc, m_vNor_loc;
    GLint m_matMVP_loc, m_matWorld_loc, m_matNor_loc;

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
        m_matWorld_loc = glGetUniformLocation (m_program, "matWorld");
        m_matNor_loc = glGetUniformLocation (m_program, "matNor");
        m_mousePos_loc = glGetUniformLocation (m_program, "mousePos");

        // Init position de la souris au milieu de la fenêtre
        int width, height;
        glfwGetWindowSize (m_window, &width, &height);
        m_mousePos = {width/2.0f, height/2.0f, (float) width, (float) height};

        // Création des objets graphiques
        m_kite1 = new Kite {m_vPos_loc, m_vCol_loc, m_vNor_loc, false};
        m_kite2 = new Kite {m_vPos_loc, m_vCol_loc, m_vNor_loc, true};
        m_sphere1 = new Sphere {m_vPos_loc, m_vCol_loc, m_vNor_loc, 1.0, 0.0, 0.0, true, 3};
        m_sphere2 = new Sphere {m_vPos_loc, m_vCol_loc, m_vNor_loc, 0.0, 1.0, 0.0, false, 3};
    }


    void tearGL()
    {
        // Destruction des objets graphiques
        delete m_kite1;
        delete m_kite2;

        glDeleteProgram (m_program);
    }


    void displayGL()
    {
        //glClearColor (0.95, 1.0, 0.8, 1.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram (m_program);

        vmath::mat4 mat_MVP;
        vmath::mat4 mat_world;
        vmath::mat3 mat_Nor;
        set_projection (mat_MVP, mat_world, mat_Nor);
        glUniformMatrix4fv (m_matMVP_loc, 1, GL_FALSE, mat_MVP);
        glUniformMatrix4fv (m_matWorld_loc, 1, GL_FALSE, mat_world);
        glUniformMatrix3fv (m_matNor_loc, 1, GL_FALSE, mat_Nor);

        // Position souris si mousePos présente dans le shader program
        if (m_mousePos_loc != -1)
            glUniform4fv (m_mousePos_loc, 1, m_mousePos);
        
        if(m_flag_phong){
            m_sphere1->draw ();
        }else{
            m_sphere2->draw ();
        }
    }


    void set_projection (vmath::mat4& mat_MVP, vmath::mat4& mat_world,
            vmath::mat3& mat_Nor)
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
        mat_world = vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f);

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
                  << "nN near  fF far  dD dist  b z-buffer  u update program  o Phong"
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
            that->reload_program ();
            break;
        case GLFW_KEY_O :
            that->m_flag_phong = !that->m_flag_phong;
            break;
        case GLFW_KEY_H :
            print_help ();
            break;
        case GLFW_KEY_ESCAPE :
            that->m_ok = false;
            break;

        case GLFW_KEY_L :
        // Toggle the fill flag
        flag_fill = !flag_fill;
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
        m_window = glfwCreateWindow (640, 480, "Spéculaire", NULL, NULL);
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