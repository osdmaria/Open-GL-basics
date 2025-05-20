/*
    OUSSADI Maria
    BOUAMARA Kamelia
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
#include <GL/glu.h>

bool flag_fill = false;

class Cylindre {
    double m_ep_cyl;         
    double m_r_cyl;          
    int m_nb_fac;            
    float m_coul_r;         
    float m_coul_v;         
    float m_coul_b;         

    std::vector<float> m_vertices;  
    std::vector<float> m_colors;   

public:
    Cylindre(double ep_cyl, double r_cyl, int nb_fac, float coul_r, float coul_v, float coul_b)
        : m_ep_cyl(ep_cyl), m_r_cyl(r_cyl), m_nb_fac(nb_fac),
          m_coul_r(coul_r), m_coul_v(coul_v), m_coul_b(coul_b) {
        generateVerticesAndColors();
    }

    void generateVerticesAndColors() {
        m_vertices.clear();
        m_colors.clear();

        // Générer les sommets et couleurs pour les deux côtés
        for (int side = -1; side <= 1; side += 2) {
            m_vertices.push_back(0.0f); // Centre
            m_vertices.push_back(0.0f);
            m_vertices.push_back(side * m_ep_cyl / 2);

            m_colors.push_back(m_coul_r);
            m_colors.push_back(m_coul_v);
            m_colors.push_back(m_coul_b);

            for (int i = 0; i <= m_nb_fac; ++i) {
                float angle = 2.0 * M_PI * i / m_nb_fac;
                float x = m_r_cyl * cos(angle);
                float y = m_r_cyl * sin(angle);
                float z = side * m_ep_cyl / 2;

                m_vertices.push_back(x);
                m_vertices.push_back(y);
                m_vertices.push_back(z);

                m_colors.push_back(m_coul_r);
                m_colors.push_back(m_coul_v);
                m_colors.push_back(m_coul_b);
            }
        }

        // Générer les sommets et couleurs pour les facettes
        for (int i = 0; i <= m_nb_fac; ++i) {
            float angle = 2.0 * M_PI * i / m_nb_fac;
            float x = m_r_cyl * cos(angle);
            float y = m_r_cyl * sin(angle);

            // Facette côté -ep_cyl/2
            m_vertices.push_back(x);
            m_vertices.push_back(y);
            m_vertices.push_back(-m_ep_cyl / 2);

            m_colors.push_back(m_coul_r * 0.8f);
            m_colors.push_back(m_coul_v * 0.8f);
            m_colors.push_back(m_coul_b * 0.8f);

            // Facette côté +ep_cyl/2
            m_vertices.push_back(x);
            m_vertices.push_back(y);
            m_vertices.push_back(m_ep_cyl / 2);

            m_colors.push_back(m_coul_r * 0.8f);
            m_colors.push_back(m_coul_v * 0.8f);
            m_colors.push_back(m_coul_b * 0.8f);
        }
    }

    void draw() {
        glPolygonMode (GL_FRONT_AND_BACK, flag_fill ? GL_FILL : GL_LINE);

        // Dessiner les deux côtés
        for (int side = 0; side < 2; ++side) {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &m_vertices[side * (m_nb_fac + 2) * 3]);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, &m_colors[side * (m_nb_fac + 2) * 3]);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glDrawArrays(GL_TRIANGLE_FAN, 0, m_nb_fac + 2);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
        }

        // Dessiner les facettes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &m_vertices[2 * (m_nb_fac + 2) * 3]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, &m_colors[2 * (m_nb_fac + 2) * 3]);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * (m_nb_fac + 1));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
};

//------------------------------------ A P P ----------------------------------

const double FRAMES_PER_SEC = 30.0;
const double ANIM_DURATION = 18.0;

// En salle TP mettre à 0 si l'affichage "bave"
const int NUM_SAMPLES = 16;

enum CamProj
{
    P_ORTHO,
    P_FRUSTUM,
    P_MAX
};

class MyApp
{
    bool m_ok = false;
    float m_alpha = 0.0f;
    GLFWwindow *m_window = nullptr;
    double m_aspect_ratio = 1.0;
    bool m_anim_flag = false;
    int m_cube_color = 2;
    float m_radius = 0.5;
    float m_anim_angle = 0, m_start_angle = 0;
    float m_cam_z, m_cam_r, m_cam_near, m_cam_far;
    bool m_depth_flag = true;
    CamProj m_cam_proj;

    const char *m_vertex_shader_text =
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

    const char *m_fragment_shader_text =
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
        auto frac_part = [](double x)
        { return x - std::floor(x); };

        // Change la coordonnée en fonction du temps
        double time = glfwGetTime(); // durée depuis init
        double slice = time / ANIM_DURATION;
        double a = frac_part(slice);
        m_anim_angle = m_start_angle + a * 360.0;
    }

    void initGL()
    {
        std::cout << __func__ << std::endl;

        glEnable(GL_DEPTH_TEST);

        const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &m_vertex_shader_text, NULL);
        compile_shader(vertex_shader, "vertex");

        const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &m_fragment_shader_text, NULL);
        compile_shader(fragment_shader, "fragment");

        m_program = glCreateProgram();
        glAttachShader(m_program, vertex_shader);
        glAttachShader(m_program, fragment_shader);
        link_program(m_program);

        // Récupère l'identifiant des "variables" dans les shaders
        m_vPos_loc = glGetAttribLocation(m_program, "vPos");
        m_vCol_loc = glGetAttribLocation(m_program, "vCol");
        m_matMVP_loc = glGetUniformLocation(m_program, "matMVP");
    }

    void displayGL()
    {
        // glClearColor (0.95, 1.0, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program);

        vmath::mat4 matrix;
        set_projection(matrix);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, matrix);

        // Dessins
        float GH = 0.4f;
        float HJ = 0.8f;


        vmath::vec3 O {1.0f, 0.0f, 0.0f};
        vmath::vec3 G {1.0f, 0.0f, 0.06f};

        // Dessiner la grande Roue
        vmath::mat4 translatedMatrix1 = matrix * vmath::translate(O);
        translatedMatrix1 = translatedMatrix1 * vmath::rotate(static_cast<float>(m_alpha * 180.0 / M_PI), 0.f, 0.0f, 1.f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix1);
        Cylindre roue(0.2f, 0.5f, 20, 0.0f, 0.0f, 1.0f);
        roue.draw();

        // Le petit cylindre au centre de la roue
        Cylindre cylindre2(0.8f, 0.05f, 20, 1.0f * 0.8, 0.0f * 0.8, 0.0f * 0.8);
        vmath::mat4 translatedMatrix11 = matrix * vmath::translate(O[0], O[1], O[2]-0.2f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix11);
        cylindre2.draw();

        vmath::vec3 H;
        H[0] = G[0] + GH * cos(m_alpha);
        H[1] = G[1] + GH * sin(m_alpha);
        H[2] = G[2];

        // Cylindre autour du point H (petit)
        vmath::mat4 translatedMatrix3 = matrix * vmath::translate(H);
        Cylindre cylindre3(0.4f,  0.05f, 20, 0.0f * 0.8, 1.0f * 0.8, 0.0f * 0.8);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix3);
        cylindre3.draw();

        // Cylindre autour du point H (grand)
        vmath::mat4 translatedMatrix4 = matrix * vmath::translate(H[0], H[1], H[2]+0.1f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix4);
        Cylindre cylindre4(0.1f, 0.1f, 20, 0.0f * 0.8, 1.0f * 0.8, 0.0f * 0.8);
        cylindre4.draw();


        // Calcule de I, J et beta
        vmath::vec3 I {H[0],G[1],G[2]};
        vmath::vec3 J{ I[0] - std::sqrt(std::pow(HJ, 2) - std::pow(GH * sin(m_alpha), 2)), 0.0f, G[2]};
        float beta = std::atan(H[1]/abs(J[0]-H[0]));

        //La barre HJ
        vmath::mat4 translatedMatrix5 = matrix * vmath::translate(J[0], J[1], J[2]+0.1f);
        translatedMatrix5 = translatedMatrix5 * vmath::rotate(static_cast<float>(beta * 180.0 / M_PI) , 0.f, 0.f, 1.f);
        translatedMatrix5 = translatedMatrix5 * vmath::translate(HJ / 2.0f, 0.f, 0.f);
        translatedMatrix5 = translatedMatrix5 * vmath::rotate(90.0f, 0.f, 1.f, 0.f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix5);
        Cylindre cylindre5(HJ, 0.05f, 20, 1.0f * 0.8, 0.0f * 0.8, 0.0f * 0.8);
        cylindre5.draw();

        //Cylindre vertical au J
        vmath::mat4 translatedMatrix6 = matrix * vmath::translate(J[0], J[1], J[2]+0.2f);
        Cylindre cylindre6(0.2f, 0.04f, 20, 0.0f * 0.8, 1.0f * 0.8, 0.0f * 0.8);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix6);
        cylindre6.draw();

        // Les deux cylindres horizontaux au J rose et vert
        vmath::mat4 translatedMatrix10 = matrix * vmath::translate(J[0], J[1], J[2]+0.1f);
        Cylindre cylindre7(0.1f, 0.1f, 20, 0.0f * 0.8, 1.0f * 0.8, 0.0f * 0.8);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix10);
        cylindre7.draw();

        Cylindre cylindre8(0.1f, 0.08f, 20, 1.0f, 1.0f * 0.55, 1.0f * 0.8);
        vmath::mat4 translatedMatrix7 = matrix * vmath::translate(J[0], J[1], J[2]+0.2f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix7);
        cylindre8.draw();

        // Barre JK
        vmath::vec3 K{G[0]-2.4f, G[1], G[2]};

        float JK = abs(vmath::length(K - J));
        vmath::vec3 centre_barre = (J+K) *0.5f;
        vmath::mat4 translatedMatrix8 = matrix * vmath::translate(centre_barre[0], centre_barre[1], centre_barre[2]+0.2f);
        translatedMatrix8 = translatedMatrix8 * vmath::rotate(90.0f, 0.f, 1.f, 0.f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix8);
        Cylindre cylindre9(JK, 0.06f, 20, 1.0f, 1.0f * 0.55, 1.0f * 0.8);
        cylindre9.draw();

        //Le piston
        vmath::mat4 translatedMatrix9 = matrix * vmath::translate(K[0], K[1], K[2]+0.2f);
        translatedMatrix9 = translatedMatrix9 * vmath::rotate(90.0f, 0.f, 1.f, 0.f);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, translatedMatrix9);
        Cylindre piston(0.4f, 0.2f, 20, 1.0f * 0.8, 0.0f * 0.8, 0.0f * 0.8);
        piston.draw();

        

    }

    void set_projection(vmath::mat4 &matrix)
    {
        matrix = vmath::mat4::identity();

        GLfloat hr = m_cam_r, wr = hr * m_aspect_ratio;
        switch (m_cam_proj)
        {
        case P_ORTHO:
            matrix = matrix * vmath::ortho(-wr, wr, -hr, hr, m_cam_near, m_cam_far);
            break;
        case P_FRUSTUM:
            matrix = matrix * vmath::frustum(-wr, wr, -hr, hr, m_cam_near, m_cam_far);
            break;
        default:;
        }

        vmath::vec3 eye{0, 0, (float)m_cam_z}, center{0, 0, 0}, up{0, 1., 0};
        matrix = matrix * vmath::lookat(eye, center, up);

        // Rotation de la scène pour l'animation, tout en float pour le template
        matrix = matrix * vmath::rotate(m_anim_angle, 0.f, 1.f, 0.15f);
    }

    void compile_shader(GLuint shader, const char *name)
    {
        std::cout << "Compile " << name << " shader...\n";
        glCompileShader(shader);

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
            m_ok = false;

        GLsizei maxLength = 2048, length;
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &length, &infoLog[0]);

        if (length == 0)
            return;
        if (isCompiled == GL_TRUE)
            std::cout << "Compilation messages:\n";
        else
            std::cout << "### Compilation errors:\n";
        for (std::size_t i = 0; infoLog[i]; i++)
            std::cout << infoLog[i];
        std::cout << std::endl;
    }

    void link_program(GLuint program)
    {
        std::cout << "Link program...\n";
        glLinkProgram(program);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
            m_ok = false;

        GLsizei maxLength = 2048, length;
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &length, &infoLog[0]);

        if (length == 0)
            return;
        if (status == GL_TRUE)
            std::cout << "Linking messages:\n";
        else
            std::cout << "### Linking errors:\n";
        for (std::size_t i = 0; infoLog[i]; i++)
            std::cout << infoLog[i];
        std::cout << std::endl;
    }

    void cam_init()
    {
        m_cam_z = 3;
        m_cam_r = 0.5;
        m_cam_near = 1;
        m_cam_far = 5;
        m_cam_proj = P_FRUSTUM;
    }

    void set_viewport(int width, int height)
    {
        glViewport(0, 0, width, height);
        m_aspect_ratio = (double)width / height;
    }

    static void on_reshape_func(GLFWwindow *window, int width, int height)
    {
        std::cout << __func__ << " "
                  << width << " " << height << std::endl;

        MyApp *that = static_cast<MyApp *>(glfwGetWindowUserPointer(window));
        that->set_viewport(width, height);
    }

    static void print_help()
    {
        std::cout << "h help  i init  a anim  p proj  zZ cam_z  rR radius  "
                  << "nN near  fF far  dD dist  b z-buffer  c cube" << std::endl;
    }

    void print_projection()
    {
        switch (m_cam_proj)
        {
        case P_ORTHO:
            std::cout << "Ortho:   ";
            break;
        case P_FRUSTUM:
            std::cout << "Frustum: ";
            break;
        default:;
        }
        GLfloat hr = m_cam_r, wr = hr * m_aspect_ratio;
        std::cout << std::fixed << std::setprecision(1)
                  << -wr << ", " << wr << ", "
                  << -hr << ", " << hr << ", "
                  << m_cam_near << ", " << m_cam_far << " ; "
                  << "cam_z = " << m_cam_z << std::endl;
    }

    static void on_key_func(GLFWwindow *window, int key, int scancode,
                            int action, int mods)
    {
        // std::cout << __func__ << " " << key << " " << scancode << " "
        //     << action << " " << mods << std::endl;

        // action = GLFW_PRESS ou GLFW_REPEAT ou GLFW_RELEASE
        if (action == GLFW_RELEASE)
            return;

        MyApp *that = static_cast<MyApp *>(glfwGetWindowUserPointer(window));

        int trans_key = translate_qwerty_to_azerty(key, scancode);
        switch (trans_key)
        {

        case GLFW_KEY_I:
            that->cam_init();
            break;
        case GLFW_KEY_A:
            that->m_anim_flag = !that->m_anim_flag;
            if (that->m_anim_flag)
            {
                that->m_start_angle = that->m_anim_angle;
                glfwSetTime(0);
            }
            break;
        case GLFW_KEY_P:
        {
            int k = static_cast<int>(that->m_cam_proj) + 1;
            if (k >= static_cast<int>(P_MAX))
                k = 0;
            that->m_cam_proj = static_cast<CamProj>(k);
            // Heuristique pour garder sensiblement la même taille
            if (that->m_cam_proj == P_FRUSTUM)
                that->m_cam_r /= 2.5;
            else if (that->m_cam_proj == P_ORTHO)
                that->m_cam_r *= 2.5;
            break;
        }
        case GLFW_KEY_Z:
            change_val_mods(that->m_cam_z, mods, 0.1, -100);
            break;
        case GLFW_KEY_R:
            change_val_mods(that->m_cam_r, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_N:
            change_val_mods(that->m_cam_near, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_F :
            // Toggle the fill flag
            flag_fill = !flag_fill;
            std::cout << "Flag fill is now " << (flag_fill ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_D:
            change_val_mods(that->m_cam_z, mods, 0.1, -100);
            change_val_mods(that->m_cam_near, mods, 0.1, 0.1);
            change_val_mods(that->m_cam_far, mods, 0.1, 0.1);
            break;
        case GLFW_KEY_B:
            that->m_depth_flag = !that->m_depth_flag;
            std::cout << "depth_flag is " << that->m_depth_flag << std::endl;
            if (that->m_depth_flag)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
            break;
        case GLFW_KEY_C:
            that->m_cube_color = (that->m_cube_color + 1) % 3;
            break;
        case GLFW_KEY_H:
            print_help();
            break;
        case GLFW_KEY_ESCAPE:
            that->m_ok = false;
            break;
        case GLFW_KEY_SPACE:
        that->m_alpha += 0.2f; // Incrémenter l'angle (ajustez la valeur selon la vitesse souhaitée)
        break;
        default:
            return;
        }

        that->print_projection();
    }

    template <typename T>
    static void change_val_mods(T &val, int mods, double incr, double min_val)
    {
        val += (mods & GLFW_MOD_SHIFT) ? incr : -incr;
        if (val <= min_val)
            val = min_val;
    }

    static int translate_qwerty_to_azerty(int key, int scancode)
    {
        // https://www.glfw.org/docs/latest/group__keys.html
        // QWERTY -> AZERTY
        switch (key)
        {
        case GLFW_KEY_Q:
            return GLFW_KEY_A;
        case GLFW_KEY_A:
            return GLFW_KEY_Q;
        case GLFW_KEY_W:
            return GLFW_KEY_Z;
        case GLFW_KEY_Z:
            return GLFW_KEY_W;
        case GLFW_KEY_SEMICOLON:
            return GLFW_KEY_M;
        }

        // Détection des différences non corrigées
        const char *name = glfwGetKeyName(key, scancode);
        if (name != NULL)
        {
            int capital = toupper(name[0]);
            if (capital != key)
            {
                std::cout << __func__ << " DIFF "
                          << capital << " " << key << std::endl;
            }
        }
        return key;
    }

    static void on_error_func(int error, const char *description)
    {
        std::cerr << "Error: " << description << std::endl;
    }

public:
    MyApp()
    {
        if (!glfwInit())
        {
            std::cerr << "GLFW: initialization failed" << std::endl;
            return;
        }
        glfwSetErrorCallback(on_error_func);

        // Hints à spécifier avant la création de la fenêtre
        //   https://www.glfw.org/docs/latest/window.html#window_hints_fb

        if (NUM_SAMPLES > 0)
            glfwWindowHint(GLFW_SAMPLES, NUM_SAMPLES);

        // On demande une version spécifique d'OpenGL
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

        // Création de la fenêtre
        m_window = glfwCreateWindow(640, 480, "Shaders et projection", NULL, NULL);
        if (!m_window)
        {
            std::cerr << "GLFW: window creation failed" << std::endl;
            return;
        }

        // Les callbacks pour GLFW étant statiques, on mémorise l'instance
        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, on_reshape_func);
        glfwSetKeyCallback(m_window, on_key_func);

        // Rend le contexte GL courant. Tous les appels GL seront placés après.
        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);
        m_ok = true;

        cam_init();
        print_help();

        // Initialisation de la machinerie GL en utilisant GLAD.
        gladLoadGL();
        std::cout << "Loaded OpenGL "
                  << GLVersion.major << "." << GLVersion.minor << std::endl;

        // Mise à jour viewport et ratio avec taille réelle de la fenêtre
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        set_viewport(width, height);

        initGL();
    }

    void run()
    {
        while (m_ok && !glfwWindowShouldClose(m_window))
        {
            displayGL();
            glfwSwapBuffers(m_window);

            if (m_anim_flag)
            {
                glfwWaitEventsTimeout(1.0 / FRAMES_PER_SEC);
                animate();
            }
            else
                glfwWaitEvents();
        }
    }

    ~MyApp()
    {
        if (m_window)
            glfwDestroyWindow(m_window);
        glfwTerminate();
    }

}; // MyApp

int main()
{
    MyApp app;
    app.run();
}
