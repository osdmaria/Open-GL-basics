/*
    Maria OUSSADI
    Kamelia BOUAMARA
*/

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// Pour générer glad.h : https://glad.dav1d.de/
//   C/C++, gl 4.5, OpenGL, Core, extensions: add all, local files
#include "glad.h"

// Pour définir des matrices : module vmath.h du OpenGL Red Book
//   https://github.com/openglredbook/examples/blob/master/include/vmath.h
// avec bugfix: erreur de signe dans Ortho().
// RQ: provoque un warning avec -O2, supprimé avec -fno-strict-aliasing
#include "vmath.h"

#include <GLFW/glfw3.h>

bool flag_fill =false;

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



//------------------------------------ A P P ----------------------------------

const double FRAMES_PER_SEC  = 30.0;
const double ANIM_DURATION   = 18.0;

// En salle TP mettre à 0 si l'affichage "bave"
const int NUM_SAMPLES = 16;

enum CamProj { P_ORTHO, P_FRUSTUM, P_MAX };


class MyApp
{
    bool m_ok = false;
    float m_alpha = 0.0f;
    GLFWwindow* m_window = nullptr;
    double m_aspect_ratio = 1.0;
    bool m_anim_flag = false;
    int m_cube_color = 1;
    float m_radius = 0.5;
    float m_anim_angle = 0,  m_start_angle = 0;
    float m_cam_z, m_cam_r, m_cam_near, m_cam_far;
    bool m_depth_flag = true;
    CamProj m_cam_proj;
    Pedale* m_pedale1 = nullptr;
    Pedale* m_pedale2 = nullptr;
    Cylindre* m_roue = nullptr;
    Cylindre* m_centre_roue = nullptr;
    Cylindre* barre1 = nullptr;
    Cylindre* barre2 = nullptr;
    Cylindre* cylindre_pedal1 = nullptr;
    Cylindre* cylindre_pedal2 = nullptr;

    const char* m_vertex_shader_text =
        "#version 330\n"
        "in vec4 vPos;\n"
        "in vec3 vCol;\n"
        "out vec3 color;\n"
        "uniform mat4 matMVP;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = matMVP * vPos;\n"
        "    color = vCol;\n"
        "}\n";

    const char* m_fragment_shader_text =
        "#version 330\n"
        "in vec3 color;\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fragColor = vec4(color, 1.0);\n"
        "}\n";

    GLuint m_program;
    GLint m_vPos_loc, m_vCol_loc;
    GLint m_matMVP_loc;


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

        const GLuint vertex_shader = glCreateShader (GL_VERTEX_SHADER);
        glShaderSource (vertex_shader, 1, &m_vertex_shader_text, NULL);
        compile_shader (vertex_shader, "vertex");

        const GLuint fragment_shader = glCreateShader (GL_FRAGMENT_SHADER);
        glShaderSource (fragment_shader, 1, &m_fragment_shader_text, NULL);
        compile_shader (fragment_shader, "fragment");
     
        m_program = glCreateProgram();
        glAttachShader (m_program, vertex_shader);
        glAttachShader (m_program, fragment_shader);
        link_program (m_program);

        // Récupère l'identifiant des "variables" dans les shaders
        m_vPos_loc = glGetAttribLocation (m_program, "vPos");
        m_vCol_loc = glGetAttribLocation (m_program, "vCol");
        m_matMVP_loc = glGetUniformLocation (m_program, "matMVP");

        // Création des objets graphiques
        m_pedale1 = new Pedale{0.4f, 0.6f, 0.2f, 0.03, 0.0f, 0.0, 1.0, m_vPos_loc, m_vCol_loc};
        m_pedale2 = new Pedale{0.4f, 0.6f, 0.2f, 0.03f, 0.0f, 0.0, 1.0, m_vPos_loc, m_vCol_loc};
        m_roue = new Cylindre{0.2f, 0.4, 36, 1.0f, 0.0f, 0.0f, m_vPos_loc, m_vCol_loc};
        m_centre_roue = new Cylindre{0.7f, 0.08f, 36, 0.0f, 1.0f, 0.0f, m_vPos_loc, m_vCol_loc};
        barre1 = new Cylindre{0.7f, 0.04f, 36, 1.0f, 0.0f, 0.0f, m_vPos_loc, m_vCol_loc};
        barre2 = new Cylindre{0.7f, 0.04f, 36, 1.0f, 0.0f, 0.0f, m_vPos_loc, m_vCol_loc};
        cylindre_pedal1 = new Cylindre{0.6f, 0.06f, 36, 0.0f, 1.0f, 0.0f, m_vPos_loc, m_vCol_loc};
        cylindre_pedal2 = new Cylindre{0.6f, 0.06f, 36, 0.0f, 1.0f, 0.0f, m_vPos_loc, m_vCol_loc};
    }


    void tearGL()
    {
        // Destruction des objets graphiques
        delete m_roue;
        delete m_centre_roue;
        delete m_pedale1;
        delete m_pedale2;
        delete barre1;
        delete barre2;
        delete cylindre_pedal1;
        delete cylindre_pedal2;

    }


    void displayGL()
    {
        //glClearColor (0.95, 1.0, 0.8, 1.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram (m_program);

        vmath::mat4 matrix;
        set_projection (matrix);
        glUniformMatrix4fv (m_matMVP_loc, 1, GL_FALSE, matrix);


        vmath::mat4 rouematrix = matrix * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, rouematrix);
        m_roue->draw();
        m_centre_roue->draw();

        vmath::mat4 barrematrix1 = matrix * vmath::translate(0.0f, 0.0f, 0.0f);
        barrematrix1 = barrematrix1 * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        barrematrix1 = barrematrix1 * vmath::translate(0.0f, 0.0f, 0.0f);
        barrematrix1 = barrematrix1 * vmath::translate(0.4f, 0.0f, -0.2f);
        barrematrix1 = barrematrix1 * vmath::rotate(90.0f ,0.0f, 1.0f, 0.0f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, barrematrix1);
        barre1->draw();

        vmath::mat4 barrematrix2 = matrix * vmath::translate(0.0f, 0.0f, 0.0f);
        barrematrix2 = barrematrix2 * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        barrematrix2 = barrematrix2 * vmath::translate(0.0f, 0.0f, 0.0f);
        barrematrix2 = barrematrix2 * vmath::translate(-0.4f, 0.0f, 0.2f);
        barrematrix2 = barrematrix2 * vmath::rotate(90.0f ,0.0f, 1.0f, 0.0f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, barrematrix2);
        barre2->draw();


        vmath::mat4 pedalmatrix1 = matrix * vmath::translate(0.0f, 0.0f, 0.0f);
        pedalmatrix1 = pedalmatrix1 * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        pedalmatrix1 = pedalmatrix1* vmath::translate(0.0f, 0.0f, 0.0f);
        pedalmatrix1 = pedalmatrix1 * vmath::translate(-0.8f, 0.0f, 0.8f);
        pedalmatrix1 = pedalmatrix1 * vmath::rotate(static_cast<float>( m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f); 
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, pedalmatrix1);
        m_pedale1->draw();

        vmath::mat4 pedalmatrix2 = matrix * vmath::translate(0.0f, 0.0f, 0.0f);
        pedalmatrix2 = pedalmatrix2 * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        pedalmatrix2 = pedalmatrix2* vmath::translate(0.0f, 0.0f, 0.0f);
        pedalmatrix2 = pedalmatrix2 * vmath::translate(0.8f, 0.0f, -0.8f);
        pedalmatrix2 = pedalmatrix2 * vmath::rotate(static_cast<float>( m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f); 
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, pedalmatrix2);
        m_pedale2->draw();

        vmath::mat4 cylindrepedalmatrix1 = matrix * vmath::translate(0.0f, 0.0f, 0.0f);
        cylindrepedalmatrix1 = cylindrepedalmatrix1 * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        cylindrepedalmatrix1 = cylindrepedalmatrix1* vmath::translate(0.0f, 0.0f, 0.0f);
        cylindrepedalmatrix1 = cylindrepedalmatrix1 * vmath::translate(0.8f, 0.0f, -0.45f);
        cylindrepedalmatrix1 = cylindrepedalmatrix1 * vmath::rotate(static_cast<float>( m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f); 
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, cylindrepedalmatrix1);
        cylindre_pedal1->draw();

        vmath::mat4 cylindrepedalmatrix2 = matrix * vmath::translate(0.0f, 0.0f, 0.0f);
        cylindrepedalmatrix2 = cylindrepedalmatrix2 * vmath::rotate(static_cast<float>( -m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f);
        cylindrepedalmatrix2 = cylindrepedalmatrix2* vmath::translate(0.0f, 0.0f, 0.0f);
        cylindrepedalmatrix2 = cylindrepedalmatrix2 * vmath::translate(-0.8f, 0.0f, 0.45f);
        cylindrepedalmatrix2 = cylindrepedalmatrix2 * vmath::rotate(static_cast<float>( m_alpha* 180.0 /M_PI), 0.0f, 0.0f, 1.0f); 
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, cylindrepedalmatrix2);
        cylindre_pedal2->draw();


    }


    void set_projection (vmath::mat4& matrix)
    {
        matrix = vmath::mat4::identity();

        GLfloat hr = m_cam_r, wr = hr * m_aspect_ratio;
        switch (m_cam_proj) {
        case P_ORTHO :
            matrix = matrix * vmath::ortho (-wr, wr, -hr, hr, m_cam_near, m_cam_far);
            break;
        case P_FRUSTUM :
            matrix = matrix * vmath::frustum (-wr, wr, -hr, hr, m_cam_near, m_cam_far);
            break;
        default: ;
        }

        vmath::vec3 eye {0, 0, (float)m_cam_z}, center {0, 0, 0}, up {0, 1., 0};
        matrix = matrix * vmath::lookat (eye, center, up);

        // Rotation de la scène pour l'animation, tout en float pour le template
        matrix = matrix * vmath::rotate (m_anim_angle, 0.f, 1.f, 0.15f);
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
                  << "nN near  fF far  dD dist  b z-buffer  c cube" << std::endl;
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
        case GLFW_KEY_L :
            // Toggle the fill flag
            flag_fill = !flag_fill;
            std::cout << "Flag fill is now " << (flag_fill ? "ON" : "OFF") << std::endl;
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
        case GLFW_KEY_H :
            print_help ();
            break;
        case GLFW_KEY_ESCAPE :
            that->m_ok = false;
            break;
        case GLFW_KEY_SPACE:
            that->m_alpha += 0.1f; // Incrémenter l'angle (ajustez la valeur selon la vitesse souhaitée)
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

public:

    MyApp()
    {
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
        m_window = glfwCreateWindow (640, 480, "Cube avec VBO", NULL, NULL);
        if (!m_window) {
            std::cerr << "GLFW: window creation failed" << std::endl;
            return;
        }

        // Les callbacks pour GLFW étant statiques, on mémorise l'instance
        glfwSetWindowUserPointer (m_window, this);
        glfwSetWindowSizeCallback (m_window, on_reshape_func);
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
        tearGL();
        if (m_window) glfwDestroyWindow (m_window);
        glfwTerminate();
    }

}; // MyApp


int main() 
{
    MyApp app;
    app.run();
}

