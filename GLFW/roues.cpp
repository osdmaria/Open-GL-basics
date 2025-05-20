/*
    Oussadi Maria
    Bouamara Kamelia
*/

#include <iostream>
#include <iomanip>
#include <cmath>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

bool flag_fill = false; 
int m_angle = 0;

class Roue
{
    int m_nb_dents;          
    double m_r_trou;   
    double m_r_roue;        
    double m_h_dent;        
    double m_coul_r, m_coul_v, m_coul_b; 
    double m_ep_roue;       

public:
    Roue(int nb_dents, double r_trou, double r_roue, double h_dent, 
         double coul_r, double coul_v, double coul_b, double ep_roue)
        : m_nb_dents(nb_dents),
          m_r_trou(r_trou),
          m_r_roue(r_roue),
          m_h_dent(h_dent),
          m_coul_r(coul_r),
          m_coul_v(coul_v),
          m_coul_b(coul_b),
          m_ep_roue(ep_roue)
    {}

    void dessiner_bloc_dent(int index)
    {
        double alpha = (2 * M_PI) / m_nb_dents; // Equ à 360 / nb_dents
        double angle_base = index * alpha;   

        // Calcul des angles pour les sommets
        double alphaA = angle_base;                        // Point A
        double alphaB = angle_base;                        // Point B
        double alphaC = angle_base + alpha / 4;       // Point C
        double alphaD = angle_base + 2 * alpha / 4;       // Point D
        double alphaE = angle_base + 3 * alpha / 4;   // Point E
        double alphaF = angle_base + alpha;   // Point F
        double alphaG = angle_base + alpha;           // Point G
        double alphaH = angle_base + 2 * alpha / 4;       // Point H

        double r_trou = m_r_trou;                          // Rayon pour A, G, H 
        double r_roue_inter = m_r_roue - m_h_dent / 2;     // Rayon pour B, F, C
        double r_roue_exter = m_r_roue + m_h_dent / 2;     // Rayon pour D, E

        // Coordonnées des sommets
        double xA = r_trou * cos(alphaA), yA = r_trou * sin(alphaA);       // Point A
        double xB = r_roue_inter * cos(alphaB), yB = r_roue_inter * sin(alphaB); // Point B
        double xC = r_roue_inter * cos(alphaC), yC = r_roue_inter * sin(alphaC); // Point C
        double xH = r_trou * cos(alphaH), yH = r_trou * sin(alphaH); // Point H
        double xD = r_roue_exter * cos(alphaD), yD = r_roue_exter * sin(alphaD); // Point D
        double xE = r_roue_exter * cos(alphaE), yE = r_roue_exter * sin(alphaE); // Point E
        double xF = r_roue_inter * cos(alphaF), yF = r_roue_inter * sin(alphaF); // Point F
        double xG = r_trou * cos(alphaG), yG = r_trou * sin(alphaG);             // Point G

        // Si flag_fill est vrai, on remplit la dent
        if (flag_fill)
        {
            glBegin(GL_TRIANGLE_FAN);
            glVertex2d(xC, yC); // C
            glVertex2d(xD, yD); // D
            glVertex2d(xE, yE); // E
            glVertex2d(xF, yF); // F
            glVertex2d(xG, yG); // G
            glVertex2d(xH, yH); // H
            glVertex2d(xA, yA); // A
            glVertex2d(xB, yB); // B
            glEnd();
        }
        else
        {
            glBegin(GL_LINE_LOOP);
            glVertex2d(xA, yA); // A
            glVertex2d(xB, yB); // B
            glVertex2d(xC, yC); // C
            glVertex2d(xD, yD); // D
            glVertex2d(xE, yE); // E
            glVertex2d(xF, yF); // F
            glVertex2d(xG, yG); // G
            glVertex2d(xH, yH); // H
            glEnd();
        }
    }


    void dessiner_cote_roue()
    {
        glPushMatrix();
        glColor3d(m_coul_r, m_coul_v, m_coul_b); 
        for (int i = 1; i <= m_nb_dents; ++i)
        {
            dessiner_bloc_dent(i);
        }
        glPopMatrix();

    }

    void dessiner_roue()
    
    {
        glPushMatrix();
        glTranslated(0.0, 0.0, m_ep_roue);
        dessiner_cote_roue(); 
        glPopMatrix();

      
        glPushMatrix();
        glTranslated(0.0, 0.0, -m_ep_roue);
        dessiner_cote_roue(); 
        glPopMatrix();
        glPushMatrix();
        for (int i = 1; i <= m_nb_dents; ++i) {
            dessiner_facettes_bloc(i);
        }
        glPopMatrix();

    }
    void dessiner_facettes_bloc(int index)
    {

    float color_perpendicular[3] = {static_cast<float>(m_coul_r * 0.7f), static_cast<float>(m_coul_v * 0.7f), static_cast<float>(m_coul_b * 0.7f)}; // Lighter Grey
    float color_oblique[3] = {static_cast<float>(m_coul_r * 0.9f), static_cast<float>(m_coul_v * 0.9f), static_cast<float>(m_coul_b * 0.9f)};      // Lighter Grey

    double alpha = (2 * M_PI) / m_nb_dents; // Equ à 360 / nb_dents
    double angle_base = index * alpha;   

    double alphaA = angle_base;                        // Point A
    double alphaB = angle_base;                        // Point B
    double alphaC = angle_base + alpha / 4;       // Point C
    double alphaD = angle_base + 2 * alpha / 4;       // Point D
    double alphaE = angle_base + 3 * alpha / 4;   // Point E
    double alphaF = angle_base + alpha;   // Point F
    double alphaG = angle_base + alpha;           // Point G
    double alphaH = angle_base + 2 * alpha / 4;       // Point H

    double r_trou = m_r_trou;                          // Rayon pour A, G, H 
    double r_roue_inter = m_r_roue - m_h_dent / 2;     // Rayon pour B, F, C
    double r_roue_exter = m_r_roue + m_h_dent / 2;     // Rayon pour D, E

    double xA = r_trou * cos(alphaA), yA = r_trou * sin(alphaA);       // Point A
    double xB = r_roue_inter * cos(alphaB), yB = r_roue_inter * sin(alphaB); // Point B
    double xC = r_roue_inter * cos(alphaC), yC = r_roue_inter * sin(alphaC); // Point C
    double xH = r_trou * cos(alphaH), yH = r_trou * sin(alphaH); // Point H
    double xD = r_roue_exter * cos(alphaD), yD = r_roue_exter * sin(alphaD); // Point D
    double xE = r_roue_exter * cos(alphaE), yE = r_roue_exter * sin(alphaE); // Point E
    double xF = r_roue_inter * cos(alphaF), yF = r_roue_inter * sin(alphaF); // Point F
    double xG = r_trou * cos(alphaG), yG = r_trou * sin(alphaG);             // Point G

    // Facet AHH'A'
    glBegin(GL_QUADS);
    glColor3fv(color_oblique);
    glVertex3f(xA, yA, m_ep_roue );
    glVertex3f(xH, yH, m_ep_roue );
    glVertex3f(xH, yH, -m_ep_roue );
    glVertex3f(xA, yA, -m_ep_roue );
    glEnd();

    // Facet HGG'H'
    glBegin(GL_QUADS);
    glColor3fv(color_oblique);
    glVertex3f(xH, yH, m_ep_roue );
    glVertex3f(xG, yG, m_ep_roue );
    glVertex3f(xG, yG, -m_ep_roue );
    glVertex3f(xH, yH, -m_ep_roue );
    glEnd();

    // Facet BCC'B'
    glBegin(GL_QUADS);
    glColor3fv(color_perpendicular);
    glVertex3f(xB, yB, m_ep_roue );
    glVertex3f(xC, yC, m_ep_roue );
    glVertex3f(xC, yC, -m_ep_roue );
    glVertex3f(xB, yB, -m_ep_roue );
    glEnd();

    // Facet CDD'C'
    glBegin(GL_QUADS);
    glColor3fv(color_perpendicular);
    glVertex3f(xC, yC, m_ep_roue );
    glVertex3f(xD, yD, m_ep_roue );
    glVertex3f(xD, yD, -m_ep_roue );
    glVertex3f(xC, yC, -m_ep_roue );
    glEnd();

    // Facet DEE'D'
    glBegin(GL_QUADS);
    glColor3fv(color_perpendicular);
    glVertex3f(xD, yD, m_ep_roue );
    glVertex3f(xE, yE, m_ep_roue );
    glVertex3f(xE, yE, -m_ep_roue );
    glVertex3f(xD, yD, -m_ep_roue );
    glEnd();

    // Facet EF F'E'
    glBegin(GL_QUADS);
    glColor3fv(color_oblique);
    glVertex3f(xE, yE, m_ep_roue );
    glVertex3f(xF, yF, m_ep_roue );
    glVertex3f(xF, yF, -m_ep_roue);
    glVertex3f(xE, yE, -m_ep_roue);
    glEnd();
}
    
    void draw()
    {   
        dessiner_roue();
    }
}; // Roue


const double FRAMES_PER_SEC  = 30.0;
const double ANIM_DURATION   = 18.0;

enum CamProj { P_ORTHO, P_FRUSTUM, P_MAX };


class MyApp
{
    bool m_ok = false;
    GLFWwindow* m_window = nullptr;
    bool m_anim_flag = false;
    double m_anim_angle = 0, m_start_angle = 0;
    double m_cam_z, m_cam_hr, m_cam_near, m_cam_far;
    double m_aspect_ratio = 1.0;
    bool m_depth_flag = true;
    int m_cube_color = 2;
    CamProj m_cam_proj;


    void animate()
    {
        // Change l'angle en fonction du temps
        double time = glfwGetTime();           // durée depuis init
        double slice = time / ANIM_DURATION;
        double a = slice - std::floor(slice);  // partie fractionnaire
        m_anim_angle = m_start_angle + a*360.0;
    }


    void displayGL()
    {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLoadIdentity();
        gluLookAt (0, 0, m_cam_z, 0, 0, 0, 0, 1, 0);
        glRotated (m_anim_angle, 0, 1.0, 0.15);

        // Dessin des différents objets
        glPushMatrix();
        glTranslated (-1.5, -0.2, 0);
        glRotated(m_angle, 0.0, 0.0, 1.0);
        Roue roue1(10, 0.5, 1.0, 0.2, 1.0, 0, 0, 0.2);
        roue1.draw();
        glPopMatrix();

        glPushMatrix();
        glTranslated (0.6, 0.3, 0);
        glRotated(-m_angle, 0.0, 0.0, 1.0);
        Roue roue2(10, 0.5, 1.0, 0.2, 0, 1.0, 0, 0.2);
        roue2.draw();
        glPopMatrix();

        glPushMatrix();
        glTranslated (-0.7, 2, 0);
        glRotated(m_angle, 0.0, 0.0, 1.0);
        Roue roue3(20, 0.3, 1.0, 0.2, 0, 0, 1.0, 0.1);
        roue3.draw();
        glPopMatrix();
    }


    void draw_wire_cube (double r)
    {
        int sa[] = {-1, -1,  1, 1},
            sb[] = {-1,  1, -1, 1};

        glColor3d (1.0, 1.0, 1.0);
        glBegin (GL_LINES);
        if (m_cube_color == 1) glColor3d (1.0, 0.0, 0.0);
        for (int i = 0; i < 4; i++) {
            glVertex3d (-r, r*sa[i], r*sb[i]);
            glVertex3d ( r, r*sa[i], r*sb[i]);
        }
        if (m_cube_color == 1) glColor3d (0.0, 1.0, 0.0);
        for (int i = 0; i < 4; i++) {
            glVertex3d (r*sa[i], -r, r*sb[i]);
            glVertex3d (r*sa[i],  r, r*sb[i]);
        }
        if (m_cube_color == 1) glColor3d (0.0, 0.0, 1.0);
        for (int i = 0; i < 4; i++) {
            glVertex3d (r*sa[i], r*sb[i], -r);
            glVertex3d (r*sa[i], r*sb[i],  r);
        }
        glEnd();
    }


    void cam_init()
    {
        m_cam_z = 3; m_cam_hr = 1; m_cam_near = 1; m_cam_far = 5; 
        m_cam_proj = P_ORTHO;
    }


    void set_projection()
    {
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity();

        double wr = m_cam_hr * m_aspect_ratio;
        switch (m_cam_proj) {
        case P_ORTHO :
            std::cout << std::fixed << std::setprecision(1) 
                      << "glOrtho (" 
                      << -wr  << ", " <<  wr  << ", " 
                      << -m_cam_hr  << ", " <<  m_cam_hr  << ", " 
                      << m_cam_near  << ", " <<  m_cam_far << ") " 
                      << "cam_z = " << m_cam_z << std::endl;
            glOrtho (-wr, wr, 
                     -m_cam_hr, m_cam_hr, 
                      m_cam_near, m_cam_far);
            break;
        case P_FRUSTUM :
            std::cout << std::fixed << std::setprecision(1) 
                      << "glFrustum (" 
                      << -wr  << ", " <<  wr  << ", " 
                      << -m_cam_hr  << ", " <<  m_cam_hr  << ", " 
                      << m_cam_near  << ", " <<  m_cam_far << ") " 
                      << "cam_z = " << m_cam_z << std::endl;
            glFrustum (-wr, wr, 
                       -m_cam_hr, m_cam_hr, 
                        m_cam_near, m_cam_far);
            break;
        default :;
        }

        glMatrixMode (GL_MODELVIEW);
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
        that->set_projection();
    }


    static void print_help()
    {
        std::cout << "h help  i init  a anim  p proj  zZ cam_z  rR radius  "
                  << "nN near  fF far  dD dist  b z-buffer  c cube" << std::endl;
    }


static void on_key_func (GLFWwindow* window, int key, int scancode, 
        int action, int mods)
{
    // std::cout << __func__ << " " << key << " " << scancode << " " 
    //    << action << " " << mods << std::endl;

    // action = GLFW_PRESS ou GLFW_REPEAT ou GLFW_RELEASE
    if (action == GLFW_RELEASE) return;

    MyApp* that = static_cast<MyApp*>(glfwGetWindowUserPointer(window));

    int trans_key = translate_qwerty_to_azerty(key, scancode);
    switch (trans_key) {

    case GLFW_KEY_I :
        that->cam_init ();
        break;
    case GLFW_KEY_A :
        that->m_anim_flag = !that->m_anim_flag;
        if (that->m_anim_flag) {
            that->m_start_angle = that->m_anim_angle;
            glfwSetTime(0);
        }
        break;
    case GLFW_KEY_P : {
        int k = static_cast<int>(that->m_cam_proj) + 1;
        if (k >= static_cast<int>(P_MAX)) k = 0;
        that->m_cam_proj = static_cast<CamProj>(k);
        break;
    }
    case GLFW_KEY_Z :
        change_val_mods(that->m_cam_z, mods, 0.1, -100);
        break;
    case GLFW_KEY_R :
        change_val_mods(that->m_cam_hr, mods, 0.1, 0.1);
        break;
    case GLFW_KEY_N :
        change_val_mods(that->m_cam_near, mods, 0.1, 0.1);
        break;
    case GLFW_KEY_F :
        // Toggle the fill flag
        flag_fill = !flag_fill;
        std::cout << "Flag fill is now " << (flag_fill ? "ON" : "OFF") << std::endl;
        break;
    case GLFW_KEY_D :
        change_val_mods(that->m_cam_z, mods, 0.1, -100);
        change_val_mods(that->m_cam_near, mods, 0.1, 0.1);
        change_val_mods(that->m_cam_far, mods, 0.1, 0.1);
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
    case GLFW_KEY_SPACE :
        m_angle++;
        break;
    default: 
        return;
    }

    that->set_projection();
}



    static void change_val_mods (double& val, int mods, double incr, double min_val)
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

        // Commenter la ligne en salle TP si l'affichage "bave"
        glfwWindowHint (GLFW_SAMPLES, 16);

        // Création de la fenêtre
        m_window = glfwCreateWindow (640, 480, "Tetraèdre", NULL, NULL);
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

        // Mise à jour viewport et ratio avec taille réelle de la fenêtre
        int width, height;
        glfwGetWindowSize (m_window, &width, &height);
        set_viewport (width, height);
        set_projection();

        glEnable (GL_DEPTH_TEST);
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
        if (m_window) glfwDestroyWindow (m_window);
        glfwTerminate();
    }

}; // MyApp


int main() 
{
    MyApp app;
    app.run();
}

