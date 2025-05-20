/*
    Mémorisation de la souris dans une variable uniforme

    CC BY-SA Edouard.Thiel@univ-amu.fr - 27/01/2025
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
// RQ: provoque un warning avec -O2, supprimé avec -fno-strict-aliasing
#include "vmath.h"

#include <GLFW/glfw3.h>

//------------------------------ T R I A N G L E S ----------------------------

class Triangles
{
    GLuint m_VAO_id, m_VBO_id;
    GLint m_vPos_loc, m_vCol_loc;

public:
    Triangles(GLint vPos_loc, GLint vCol_loc)
        : m_vPos_loc{vPos_loc}, m_vCol_loc{vCol_loc}
    {
        // Données
        GLfloat positions[] = {
            -0.7, -0.5, -0.1,
            0.8, -0.2, -0.1,
            0.1, 0.9, 0.3,
            -0.6, 0.7, -0.2,
            0.8, 0.8, -0.2,
            0.1, -0.9, 0.7};

        GLfloat colors[] = {
            1.0, 0.6, 0.6,
            1.0, 0.6, 0.6,
            1.0, 0.6, 0.6,
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0};

        // Création d'une structure de données à plat
        std::vector<GLfloat> vertices;
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < 3; j++)
                vertices.push_back(positions[i * 3 + j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back(colors[i * 3 + j]);
        }

        // Création du VAO
        glCreateVertexArrays(1, &m_VAO_id);
        glBindVertexArray(m_VAO_id);

        // Création du VBO pour les positions et couleurs
        glGenBuffers(1, &m_VBO_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO_id);

        // Copie le buffer dans la mémoire du serveur
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                     vertices.data(), GL_STATIC_DRAW);

        // VAA associant les données à la variable vPos du shader, avec l'offset 0
        glVertexAttribPointer(m_vPos_loc, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(GLfloat), reinterpret_cast<void *>(0 * sizeof(GLfloat)));
        glEnableVertexAttribArray(m_vPos_loc);

        // VAA associant les données à la variable vCol du shader, avec l'offset 3
        glVertexAttribPointer(m_vCol_loc, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(m_vCol_loc);

        glBindVertexArray(0); // désactive le VAO courant m_VAO_id
    }

    ~Triangles()
    {
        glDeleteBuffers(1, &m_VBO_id);
        glDeleteVertexArrays(1, &m_VAO_id);
    }

    void draw()
    {
        glBindVertexArray(m_VAO_id);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

}; // Triangles

//------------------------------ W I R E   C U B E ----------------------------

class WireCube
{
    bool m_is_white;
    GLfloat m_radius;
    GLuint m_VAO_id, m_VBO_id, m_EBO_id;
    GLint m_vPos_loc, m_vCol_loc;

public:
    WireCube(bool is_white, GLfloat radius, GLint vPos_loc, GLint vCol_loc)
        : m_is_white{is_white}, m_radius{radius},
          m_vPos_loc{vPos_loc}, m_vCol_loc{vCol_loc}
    {
        GLfloat r = m_radius;

        // Positions
        GLfloat positions[] = {
            -r, -r, -r, // P0                 6 ------- 7
            r, -r, -r,  // P1               / |       / |
            -r, r, -r,  // P2             /   |     /   |
            r, r, -r,   // P3           2 ------- 3     |
            -r, -r, r,  // P4           |     4 --|---- 5
            r, -r, r,   // P5           |   /     |   /
            -r, r, r,   // P6           | /       | /
            r, r, r,    // P7           0 ------- 1
        };

        // Indices : positions, couleurs
        GLint indexes[] = {
            0, 1, 0, 0,
            2, 3, 0, 0,
            4, 5, 0, 0,
            6, 7, 0, 0,
            0, 2, 1, 1,
            1, 3, 1, 1,
            4, 6, 1, 1,
            5, 7, 1, 1,
            0, 4, 2, 2,
            1, 5, 2, 2,
            2, 6, 2, 2,
            3, 7, 2, 2};

        // Couleurs
        GLfloat colors_rgb[] = {
            1.0, 0.0, 0.0, // C0       C1  C2
            0.0, 1.0, 0.0, // C1       | /
            0.0, 0.0, 1.0  // C2       + --C0
        };
        GLfloat colors_white[] = {
            1.0, 1.0, 1.0, // C0
            1.0, 1.0, 1.0, // C1
            1.0, 1.0, 1.0  // C2
        };
        GLfloat *colors = (is_white) ? colors_white : colors_rgb;

        // Création d'une structure de données à plat
        std::vector<GLfloat> vertices;
        for (int i = 0; i < 12 * 4; i += 4)
        {
            for (int j = 0; j < 3; j++)
                vertices.push_back(positions[indexes[i + 0] * 3 + j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back(colors[indexes[i + 2] * 3 + j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back(positions[indexes[i + 1] * 3 + j]);
            for (int j = 0; j < 3; j++)
                vertices.push_back(colors[indexes[i + 3] * 3 + j]);
        }

        // Création du VAO
        glCreateVertexArrays(1, &m_VAO_id);
        glBindVertexArray(m_VAO_id);

        // Création du VBO pour les positions et couleurs
        glGenBuffers(1, &m_VBO_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO_id);

        // Copie le buffer dans la mémoire du serveur
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                     vertices.data(), GL_STATIC_DRAW);

        // VAA associant les données à la variable vPos du shader, avec l'offset 0
        glVertexAttribPointer(m_vPos_loc, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(GLfloat), reinterpret_cast<void *>(0 * sizeof(GLfloat)));
        glEnableVertexAttribArray(m_vPos_loc);

        // VAA associant les données à la variable vCol du shader, avec l'offset 3
        glVertexAttribPointer(m_vCol_loc, 3, GL_FLOAT, GL_FALSE,
                              6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(m_vCol_loc);

        glBindVertexArray(0); // désactive le VAO courant m_VAO_id
    }

    ~WireCube()
    {
        glDeleteBuffers(1, &m_VBO_id);
        glDeleteVertexArrays(1, &m_VAO_id);
    }

    void draw()
    {
        glBindVertexArray(m_VAO_id);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);
    }

}; // WireCube

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
    GLFWwindow *m_window = nullptr;
    double m_aspect_ratio = 1.0;
    bool m_anim_flag = false;
    int m_cube_color = 1;
    float m_radius = 0.5;
    float m_anim_angle = 0, m_start_angle = 0;
    float m_cam_z, m_cam_r, m_cam_near, m_cam_far;
    bool m_depth_flag = true;
    CamProj m_cam_proj;
    Triangles *m_triangles = nullptr;
    WireCube *m_wire_cube_rgb = nullptr;
    WireCube *m_wire_cube_white = nullptr;
    // Exercice 1 .3 ajout de la va de temps
    GLint m_uTime_loc;
    const char *m_default_vertex_shader_text =
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

    const char *m_default_fragment_shader_text =
        "#version 330\n"
        "in vec3 color;\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fragColor = vec4(color, 1.0);\n"
        "}\n";

    std::string m_vertex_shader_path, m_fragment_shader_path;

    GLuint m_program;
    GLint m_vPos_loc, m_vCol_loc;
    GLint m_matMVP_loc;

    vmath::vec4 m_mousePos; // mouse_x, mouse_y, width, height
    GLint m_mousePos_loc;

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

        m_program = load_and_compile_program(m_vertex_shader_path,
                                             m_fragment_shader_path);

        // Récupère l'identifiant des "variables" dans les shaders
        m_vPos_loc = glGetAttribLocation(m_program, "vPos");
        m_vCol_loc = glGetAttribLocation(m_program, "vCol");
        m_matMVP_loc = glGetUniformLocation(m_program, "matMVP");
        m_mousePos_loc = glGetUniformLocation(m_program, "mousePos");

        // Init position de la souris au milieu de la fenêtre
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        m_mousePos = {width / 2.0f, height / 2.0f, (float)width, (float)height};

        // Création des objets graphiques
        m_triangles = new Triangles{m_vPos_loc, m_vCol_loc};
        m_wire_cube_white = new WireCube{true, 0.5, m_vPos_loc, m_vCol_loc};
        m_wire_cube_rgb = new WireCube{false, 0.5, m_vPos_loc, m_vCol_loc};
        // initiation de utiem
        m_uTime_loc = glGetUniformLocation(m_program, "uTime");
    }

    void tearGL()
    {
        // Destruction des objets graphiques
        delete m_triangles;
        delete m_wire_cube_white;
        delete m_wire_cube_rgb;

        glDeleteProgram(m_program);
    }

    void displayGL()
    {
        // glClearColor (0.95, 1.0, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program);

        vmath::mat4 matrix;
        set_projection(matrix);
        glUniformMatrix4fv(m_matMVP_loc, 1, GL_FALSE, matrix);

        // Position souris si mousePos présente dans le shader program
        if (m_mousePos_loc != -1)
            glUniform4fv(m_mousePos_loc, 1, m_mousePos);

        // Dessins
        m_triangles->draw();

        if (m_cube_color == 1)
            m_wire_cube_white->draw();
        else if (m_cube_color == 2)
            m_wire_cube_rgb->draw();
        if (m_uTime_loc != -1)
            glUniform1f(m_uTime_loc, glfwGetTime()); // Envoi du temps au shader
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
        char infoLog[maxLength];
        glGetShaderInfoLog(shader, maxLength, &length, infoLog);

        if (length == 0)
            return;
        if (isCompiled == GL_TRUE)
            std::cout << "Compilation messages:\n";
        else
            std::cout << "### Compilation errors:\n";
        std::cout << infoLog << std::endl;
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
        char infoLog[maxLength];
        glGetProgramInfoLog(program, maxLength, &length, infoLog);

        if (length == 0)
            return;
        if (status == GL_TRUE)
            std::cout << "Linking messages:\n";
        else
            std::cout << "### Linking errors:\n";
        std::cout << infoLog << std::endl;
    }

    std::string load_shader_code(const std::string path, const char *default_text)
    {
        if (path == "")
            return std::string(default_text);

        std::string shader_code;
        std::ifstream shader_file;
        std::stringstream shader_stream;

        shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            std::cout << "Loading " << path << " ..." << std::endl;
            shader_file.open(path);
            shader_stream << shader_file.rdbuf();
            shader_file.close();
            shader_code = shader_stream.str();
        }
        catch (...)
        {
            std::cerr << "### Load error: " << strerror(errno) << std::endl;
            return std::string(default_text);
        }

        return shader_code;
    }

    GLuint load_and_compile_program(const std::string vertex_shader_path,
                                    const std::string fragment_shader_path)
    {
        std::string vertex_shader_code = load_shader_code(vertex_shader_path,
                                                          m_default_vertex_shader_text);
        std::string fragment_shader_code = load_shader_code(fragment_shader_path,
                                                            m_default_fragment_shader_text);

        const char *vertex_shader_text = vertex_shader_code.c_str();
        const char *fragment_shader_text = fragment_shader_code.c_str();

        const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
        compile_shader(vertex_shader, "vertex");

        const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
        compile_shader(fragment_shader, "fragment");

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        link_program(program);

        // Marque les shaders pour suppression lorsque glDeleteProgram sera appelé
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        return program;
    }

    void reload_program()
    {
        glDeleteProgram(m_program);
        m_program = load_and_compile_program(m_vertex_shader_path,
                                             m_fragment_shader_path);
        m_mousePos_loc = glGetUniformLocation(m_program, "mousePos");
        m_uTime_loc = glGetUniformLocation(m_program, "uTime");
        std::cout << "Uniform mousePos " << ((m_mousePos_loc == -1) ? "not found" : "found") << std::endl;
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
                  << "nN near  fF far  dD dist  b z-buffer  c cube  u update program"
                  << std::endl;
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

    static void on_mouse_func(GLFWwindow *window, double xpos, double ypos)
    {
        // std::cout << __func__ << " " << xpos << " " << ypos << std::endl;

        MyApp *that = static_cast<MyApp *>(glfwGetWindowUserPointer(window));
        if (!that->m_ok)
            return;

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        that->m_mousePos[0] = xpos;
        that->m_mousePos[1] = height - ypos; // origine en bas à gauche
        that->m_mousePos[2] = width;
        that->m_mousePos[3] = height;
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
        case GLFW_KEY_F:
            change_val_mods(that->m_cam_far, mods, 0.1, 0.1);
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
        case GLFW_KEY_U:
            that->reload_program();
            break;
        case GLFW_KEY_H:
            print_help();
            break;
        case GLFW_KEY_ESCAPE:
            that->m_ok = false;
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

    bool parse_args(int argc, char *argv[])
    {
        int i = 1;
        while (i < argc)
        {
            if (strcmp(argv[i], "-vs") == 0 && i + 1 < argc)
            {
                m_vertex_shader_path = argv[i + 1];
                i += 2;
                continue;
            }
            if (strcmp(argv[i], "-fs") == 0 && i + 1 < argc)
            {
                m_fragment_shader_path = argv[i + 1];
                i += 2;
                continue;
            }
            if (strcmp(argv[i], "--help") == 0)
            {
                std::cout << "Options: -vs vs_file -fs fs_file\n";
                return false;
            }
            std::cerr << "Error, bad arguments. Try --help" << std::endl;
            return false;
        }
        return true;
    }

public:
    MyApp(int argc, char *argv[])
    {
        if (!parse_args(argc, argv))
            return;

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
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        // glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Création de la fenêtre
        m_window = glfwCreateWindow(640, 480, "Mouse in uniform", NULL, NULL);
        if (!m_window)
        {
            std::cerr << "GLFW: window creation failed" << std::endl;
            return;
        }

        // Les callbacks pour GLFW étant statiques, on mémorise l'instance
        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, on_reshape_func);
        glfwSetCursorPosCallback(m_window, on_mouse_func);
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
        tearGL();
        if (m_window)
            glfwDestroyWindow(m_window);
        glfwTerminate();
    }

}; // MyApp

int main(int argc, char *argv[])
{
    MyApp app{argc, argv};
    app.run();
}
