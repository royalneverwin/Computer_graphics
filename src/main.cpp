// Hand Example
// Author: Yi Kangrui <yikangrui@pku.edu.cn>

//#define DIFFUSE_TEXTURE_MAPPING

#include "gl_env.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <config.h>

#ifndef M_PI
#define M_PI (3.1415926535897932)
#endif

#include <iostream>

#include "skeletal_mesh.h"

#include <glm/gtc/matrix_transform.hpp>

namespace SkeletalAnimation {
    const char *vertex_shader_330 =
            "#version 330 core\n"
            "const int MAX_BONES = 100;\n"
            "uniform mat4 u_bone_transf[MAX_BONES];\n"
            "uniform mat4 u_mvp;\n"
            "layout(location = 0) in vec3 in_position;\n"
            "layout(location = 1) in vec2 in_texcoord;\n"
            "layout(location = 2) in vec3 in_normal;\n"
            "layout(location = 3) in ivec4 in_bone_index;\n"
            "layout(location = 4) in vec4 in_bone_weight;\n"
            "out vec2 pass_texcoord;\n"
            "void main() {\n"
            "    float adjust_factor = 0.0;\n"
            "    for (int i = 0; i < 4; i++) adjust_factor += in_bone_weight[i] * 0.25;\n"
            "    mat4 bone_transform = mat4(1.0);\n"
            "    if (adjust_factor > 1e-3) {\n"
            "        bone_transform -= bone_transform;\n"
            "        for (int i = 0; i < 4; i++)\n"
            "            bone_transform += u_bone_transf[in_bone_index[i]] * in_bone_weight[i] / adjust_factor;\n"
            "	 }\n"
            "    gl_Position = u_mvp * bone_transform * vec4(in_position, 1.0);\n"
            "    pass_texcoord = in_texcoord;\n"
            "}\n";

    const char *fragment_shader_330 =
            "#version 330 core\n"
            "uniform sampler2D u_diffuse;\n"
            "in vec2 pass_texcoord;\n"
            "out vec4 out_color;\n"
            "void main() {\n"
            #ifdef DIFFUSE_TEXTURE_MAPPING
            "    out_color = vec4(texture(u_diffuse, pass_texcoord).xyz, 1.0);\n"
            #else
            "    out_color = vec4(pass_texcoord, 0.0, 1.0);\n"
            #endif
            "}\n";
}

/*动作种类*/
enum Motion{
    PAUSE = -1,
    GRASP = 1,
    WAVE,
    OK,
    BINGO,
    GUN,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE
};
Motion curMotion = GRASP;
Motion motionBeforePause = GRASP;


/*Manage Camera*/
float lastFrameTime = 0.0f;
float deltaTime = 0.0f;
float cameraSpeed = 0.0f;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);//camera位置
/*前方和上方叉乘即可得到右方向量*/
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);//camera的前方（物体位置）
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);//camera的上方

/*Manage Pause*/
float pauseTimeOffset = 0;
float rsv_passed_time = 0;

/*Manage Transtion*/
float passed_time = 0.0f;
float cur_passed_time = 0.0f;
int bingoFlag = 0;

/*Manage Mouse Contorl*/
float pitch = 0.0f, yaw = 0.0f;//欧拉角, roll会让用户头晕, 因此不用
float lastX, lastY;
bool firstMouse = true;

/*控制缩放*/
float zoom = 1.0f;

static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

/*键盘交互*/
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS){
        cameraSpeed = 500.0f*deltaTime;
        switch(key){
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_W://图像向上
                cameraPos -= cameraSpeed * cameraUp;
                break;
            case GLFW_KEY_S://图像向下
                cameraPos += cameraSpeed * cameraUp;
                break;
            case GLFW_KEY_A://图像向左
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                break;
            case GLFW_KEY_D://图像向右
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                break;
            case GLFW_KEY_G://GRASP
                curMotion = GRASP;
                break;
            case GLFW_KEY_V://WAVE
                curMotion = WAVE;
                break;
            case GLFW_KEY_O://OK
                curMotion = OK;
                break;
            case GLFW_KEY_B://BINGO
                bingoFlag = 0;
                cur_passed_time = passed_time;
                curMotion = BINGO;
                break;
            case GLFW_KEY_U://GUN
                bingoFlag = 0;
                cur_passed_time = passed_time;
                curMotion = GUN;
                break;
            case GLFW_KEY_1:
                curMotion = ONE;
                break;
            case GLFW_KEY_2:
                curMotion = TWO;
                break;
            case GLFW_KEY_3:
                curMotion = THREE;
                break;
            case GLFW_KEY_4:
                curMotion = FOUR;
                break;
            case GLFW_KEY_5:
                curMotion = FIVE;
                break;
            case GLFW_KEY_6:
                curMotion = SIX;
                break;
            case GLFW_KEY_7:
                curMotion = SEVEN;
                break;
            case GLFW_KEY_8:
                bingoFlag = 0;
                cur_passed_time = passed_time;
                curMotion = EIGHT;
                break;
            case GLFW_KEY_9:
                curMotion = NINE;
                break;
            case GLFW_KEY_SPACE:
                if(curMotion != PAUSE) {
                    motionBeforePause = curMotion;
                    curMotion = PAUSE;
                } else{
                    curMotion = motionBeforePause;
                }
            default:
                break;
        }
    }
}

/*鼠标交互*/
static void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if(firstMouse){
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;//注意这个
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.5;
    xOffset *= sensitivity;
    yOffset *= sensitivity;
    yaw   += xOffset;
    pitch += yOffset;
    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}


//鼠标滚轮回调
//static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
//    if(zoom >= 0.1f && zoom <= 100.0f){
//        zoom -= yoffset;
//    } else if(zoom < 0.1f){
//        zoom = 0.1f;
//    } else{
//        zoom = 100.0f;
//    }
//}

/*鼠标按键交互*/
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if(action == GLFW_PRESS){
        switch(button){
            case GLFW_MOUSE_BUTTON_LEFT:
                zoom -= 0.1f;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                zoom += 0.1f;
            default:
                break;
        }
        if(zoom < 0.1f){
            zoom = 0.1f;
        } else if(zoom > 100.0f){
            zoom = 100.0f;
        }
    }
}


int main(int argc, char *argv[]) {
    GLFWwindow *window;
    GLuint vertex_shader, fragment_shader, program;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__ // for macos
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(800, 800, "OpenGL output", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK)
        exit(EXIT_FAILURE);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &SkeletalAnimation::vertex_shader_330, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &SkeletalAnimation::fragment_shader_330, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int linkStatus;
    if (glGetProgramiv(program, GL_LINK_STATUS, &linkStatus), linkStatus == GL_FALSE)
        std::cout << "Error occured in glLinkProgram()" << std::endl;

    SkeletalMesh::Scene &sr = SkeletalMesh::Scene::loadScene("Hand", DATA_DIR"/Hand.fbx");
    if (&sr == &SkeletalMesh::Scene::error)
        std::cout << "Error occured in loadMesh()" << std::endl;

    sr.setShaderInput(program, "in_position", "in_texcoord", "in_normal", "in_bone_index", "in_bone_weight");

    SkeletalMesh::SkeletonModifier modifier;

    glEnable(GL_DEPTH_TEST);
    while (!glfwWindowShouldClose(window)) {
//        passed_time = (float) glfwGetTime();

        // --- You may edit below ---

        /*Control Camera*/
        float curFrameTime = (float) glfwGetTime();
        deltaTime = curFrameTime - lastFrameTime;
        lastFrameTime = curFrameTime;


        /* Manage Pause
         * 要确保pause之后图像可以平滑过渡到转动态, 因此passed_time得改, 要确保passed_time在pause这段时间不要动
         */
        float prv_passed_time = rsv_passed_time;
        rsv_passed_time = clock() / (float) CLOCKS_PER_SEC;
        passed_time = rsv_passed_time - pauseTimeOffset;
        if (curMotion == PAUSE) {
            pauseTimeOffset += rsv_passed_time - prv_passed_time;//每次while循环一次花费的时间
        }


        // Example: Rotate the hand
        // * turn around every 4 seconds
        float metacarpals_angle = passed_time * (M_PI / 4.0f);
        // * target = metacarpals
        // * rotation axis = (1, 0, 0)
//        if (curMotion != WAVE && curMotion != PAUSE)
//            modifier["metacarpals"] = glm::rotate(glm::identity<glm::mat4>(), metacarpals_angle,
//                                                  glm::fvec3(1.0, 0.0, 0.0));

        /**********************************************************************************\
        *
        * To animate fingers, modify modifier["HAND_SECTION"] each frame,
        * where HAND_SECTION can only be one of the bone names in the Hand's Hierarchy.
        *
        * A virtual hand's structure is like this: (slightly DIFFERENT from the real world)
        *    5432 1
        *    ....        1 = thumb           . = fingertip
        *    |||| .      2 = index finger    | = distal phalange
        *    $$$$ |      3 = middle finger   $ = intermediate phalange
        *    #### $      4 = ring finger     # = proximal phalange
        *    OOOO#       5 = pinky           O = metacarpals
        *     OOO
        * (Hand in the real world -> https://en.wikipedia.org/wiki/Hand)
        *
        * From the structure we can infer the Hand's Hierarchy:
        *	- metacarpals
        *		- thumb_proximal_phalange
        *			- thumb_intermediate_phalange
        *				- thumb_distal_phalange
        *					- thumb_fingertip
        *		- index_proximal_phalange
        *			- index_intermediate_phalange
        *				- index_distal_phalange
        *					- index_fingertip
        *		- middle_proximal_phalange
        *			- middle_intermediate_phalange
        *				- middle_distal_phalange
        *					- middle_fingertip
        *		- ring_proximal_phalange
        *			- ring_intermediate_phalange
        *				- ring_distal_phalange
        *					- ring_fingertip
        *		- pinky_proximal_phalange
        *			- pinky_intermediate_phalange
        *				- pinky_distal_phalange
        *					- pinky_fingertip
        *
        * Notice that modifier["HAND_SECTION"] is a local transformation matrix,
        * where (1, 0, 0) is the bone's direction, and apparently (0, 1, 0) / (0, 0, 1)
        * is perpendicular to the bone.
        * Particularly, (0, 0, 1) is the rotation axis of the nearer joint.
        *
        \**********************************************************************************/
        // Example: Animate the index finger
        // * period = 2.4 seconds
        float period = 2.4f;
        float time_in_period = fmod(passed_time, period);
        // * angle: 0 -> PI/3 -> 0
        float thumb_angle;
        // * target = proximal phalange of the index
        // * rotation axis = (0, 0, 1)
        float thumb_angle2;
        /*抓握*/
        if (curMotion == GRASP) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 2.3);
            thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 7);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*挥手*/
        else if (curMotion == WAVE) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3);
            modifier["metacarpals"] = glm::rotate(glm::identity<glm::mat4>(), float(thumb_angle - M_PI / 6),
                                                  glm::fvec3(0.0, 1.0, 0.0));
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*OK手势*/
        else if (curMotion == OK) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3);
            thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 8.3);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                              glm::fvec3(-0.5, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                                  glm::fvec3(-0.3, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(thumb_angle * 0.6),
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*点赞*/
        else if (curMotion == BINGO) {
            float time_in_bingo = passed_time - cur_passed_time + period * 0.5f;
            float bingo;
            if (time_in_bingo >= period) {
                bingoFlag = 1;
            }
            if (!bingoFlag) {
                bingo = abs(fmod(time_in_bingo, period) / (period * 0.5f) - 1.0f) * (M_PI / 2);
            } else {
                bingo = M_PI / 2;
            }
            modifier["metacarpals"] = glm::rotate(glm::identity<glm::mat4>(), bingo, glm::fvec3(0.0, 1.0, 0.0));
            if (bingoFlag) {
                thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3);
                thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3);
                modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), -thumb_angle2,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.1, 0.0, 1.0));
                modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                          glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                       glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                     glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                               glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                         glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                          glm::fvec3(0.0, 0.0, 1.0));
            }
        }
            /*手枪*/
        else if (curMotion == GUN) {
            float time_in_bingo = passed_time - cur_passed_time + period * 0.5f;
            if (time_in_bingo >= period) {
                bingoFlag = 1;
            }
            thumb_angle = abs(time_in_bingo / (period * 0.5f) - 1.0f) * (M_PI / 2.5);
            thumb_angle2 = abs(time_in_bingo / (period * 0.5f) - 1.0f) * (M_PI / 6);
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            if (!bingoFlag) {
                modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), -thumb_angle2,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                       glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                     glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                               glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                         glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                          glm::fvec3(0.0, 0.0, 1.0));
            } else {
                modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), -float(M_PI / 6.0),
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                   glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                       glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                           glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                     glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                               glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                         glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                  glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), float(M_PI / 2.5),
                                                          glm::fvec3(0.0, 0.0, 1.0));
            }
        }
            /*数字1*/
        else if (curMotion == ONE) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 2.5);
            thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 7);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*数字2*/
        else if (curMotion == TWO) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 2.5);
            thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 7);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*数字3*/
        else if (curMotion == THREE) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 5);
            thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 4);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*数字4*/
        else if (curMotion == FOUR) {
            thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 4);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle2,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
            /*数字5*/
        else if (curMotion == FIVE) {
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
        /*数字6*/
        if (curMotion == SIX) {
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 2.3);
            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                              glm::fvec3(0.1, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                      glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                               glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                       glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                             glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                     glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                              glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                            glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                      glm::fvec3(0.0, 0.0, 1.0));
        }
        /*数字7*/
        else if(curMotion == SEVEN){
            float angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 8.0);
            float flat_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 4.0);
            float other_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
            float thumb_distal_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 36.0);
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 8.0);

            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), flat_angle, glm::fvec3(0.07, -0.07, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), flat_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));

            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));

            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_distal_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
        }
        /*数字8*/
        else if (curMotion == EIGHT) {
            float time_in_bingo = passed_time - cur_passed_time + period * 0.5f;
            float bingo;
            if (time_in_bingo >= period) {
                bingoFlag = 1;
            }
            if (!bingoFlag) {
                bingo = abs(fmod(time_in_bingo, period) / (period * 0.5f) - 1.0f) * (M_PI / 2);
            } else {
                bingo = M_PI / 2;
            }
            modifier["metacarpals"] = glm::rotate(glm::identity<glm::mat4>(), bingo, glm::fvec3(0.0, 1.0, 0.0));
            if (bingoFlag) {
                thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 2.5);
                thumb_angle2 = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3);
                modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), -thumb_angle2,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.1, 0.0, 1.0));
                modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                          glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                       glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                           glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                     glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                               glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                         glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle,
                                                          glm::fvec3(0.0, 0.0, 1.0));
            } else {
                modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.1, 0.0, 1.0));
                modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["thumb_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                          glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.1, 0.0, 1.0));
                modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["index_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                          glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                   glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                       glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["middle_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                           glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                 glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                     glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                               glm::fvec3(0.0, 0.0, 1.0));
                modifier["ring_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                         glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                  glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                      glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                                glm::fvec3(0.0, 0.0, 1.0));
                modifier["pinky_fingertip"] = glm::rotate(glm::identity<glm::mat4>(), 0.0f,
                                                          glm::fvec3(0.0, 0.0, 1.0));
            }
        }
        /*数字9*/
        else if(curMotion == NINE){
            float angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 5.0);
            float index_prox_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 36.0);
            float other_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 2.5);
            thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 6.0);

            modifier["thumb_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), index_prox_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_proximal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));

            modifier["thumb_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_intermediate_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));

            modifier["thumb_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["index_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["middle_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["ring_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
            modifier["pinky_distal_phalange"] = glm::rotate(glm::identity<glm::mat4>(), other_angle, glm::fvec3(0.0, 0.0, 1.0));
        }
        // --- You may edit above ---

        float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glClearColor(0.5, 0.5, 0.5, 1.0);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glm::fmat4 mvp = glm::ortho(-12.5f * ratio*zoom, 12.5f * ratio*zoom, -5.f*zoom, 20.f*zoom, -20.f, 20.f)
                         *
//                         glm::lookAt(glm::fvec3(.0f, .0f, -1.f), glm::fvec3(.0f, .0f, .0f), glm::fvec3(.0f, 1.f, .0f));
                            /*修改了这里实现视角移动*/
                         glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, (const GLfloat *) &mvp);
        glUniform1i(glGetUniformLocation(program, "u_diffuse"), SCENE_RESOURCE_SHADER_DIFFUSE_CHANNEL);
        SkeletalMesh::Scene::SkeletonTransf bonesTransf;
        sr.getSkeletonTransform(bonesTransf, modifier);
        if (!bonesTransf.empty())
            glUniformMatrix4fv(glGetUniformLocation(program, "u_bone_transf"), bonesTransf.size(), GL_FALSE,
                               (float *) bonesTransf.data());
        sr.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    SkeletalMesh::Scene::unloadScene("Hand");

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}