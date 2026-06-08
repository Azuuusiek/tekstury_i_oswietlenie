#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "linmath.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::string read_entire_file(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compile_shader_from_file(const char* path, GLenum type) {
    std::string src = read_entire_file(path);
    if (src.empty()) return 0;

    GLuint s = glCreateShader(type);
    const char* cstr = src.c_str();
    glShaderSource(s, 1, &cstr, NULL);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        std::cerr << "Shader compile error (" << path << "):\n" << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint link_program(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, sizeof(log), NULL, log);
        std::cerr << "Program link error:\n" << log << "\n";
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

// ----------- Vertex struct ----------
struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

static Vertex cube_vertices[] = {
    {-0.5f,-0.5f, 0.5f,  0,0,1,  0.0f,0.0f,  1,0,0},
    { 0.5f,-0.5f, 0.5f,  0,0,1,  1.0f,0.0f,  1,0,0},
    { 0.5f, 0.5f, 0.5f,  0,0,1,  1.0f,1.0f,  1,0,0},
    {-0.5f, 0.5f, 0.5f,  0,0,1,  0.0f,1.0f,  1,0,0},

    { 0.5f,-0.5f,-0.5f,  0,0,-1, 0.0f,0.0f, -1,0,0},
    {-0.5f,-0.5f,-0.5f,  0,0,-1, 1.0f,0.0f, -1,0,0},
    {-0.5f, 0.5f,-0.5f,  0,0,-1, 1.0f,1.0f, -1,0,0},
    { 0.5f, 0.5f,-0.5f,  0,0,-1, 0.0f,1.0f, -1,0,0},

    {-0.5f,-0.5f,-0.5f, -1,0,0, 0.0f,0.0f,  0,0,-1},
    {-0.5f,-0.5f, 0.5f, -1,0,0, 1.0f,0.0f,  0,0,-1},
    {-0.5f, 0.5f, 0.5f, -1,0,0, 1.0f,1.0f,  0,0,-1},
    {-0.5f, 0.5f,-0.5f, -1,0,0, 0.0f,1.0f,  0,0,-1},

    { 0.5f,-0.5f, 0.5f,  1,0,0, 0.0f,0.0f,  0,0,1},
    { 0.5f,-0.5f,-0.5f,  1,0,0, 1.0f,0.0f,  0,0,1},
    { 0.5f, 0.5f,-0.5f,  1,0,0, 1.0f,1.0f,  0,0,1},
    { 0.5f, 0.5f, 0.5f,  1,0,0, 0.0f,1.0f,  0,0,1},

    {-0.5f, 0.5f, 0.5f,  0,1,0, 0.0f,0.0f,  1,0,0},
    { 0.5f, 0.5f, 0.5f,  0,1,0, 1.0f,0.0f,  1,0,0},
    { 0.5f, 0.5f,-0.5f,  0,1,0, 1.0f,1.0f,  1,0,0},
    {-0.5f, 0.5f,-0.5f,  0,1,0, 0.0f,1.0f,  1,0,0},

    {-0.5f,-0.5f,-0.5f,  0,-1,0, 0.0f,0.0f,  1,0,0},
    { 0.5f,-0.5f,-0.5f,  0,-1,0, 1.0f,0.0f,  1,0,0},
    { 0.5f,-0.5f, 0.5f,  0,-1,0, 1.0f,1.0f,  1,0,0},
    {-0.5f,-0.5f, 0.5f,  0,-1,0, 0.0f,1.0f,  1,0,0},
};

static unsigned short cube_indices[] = {
    0,1,2, 2,3,0,
    4,5,6, 6,7,4,
    8,9,10, 10,11,8,
    12,13,14, 14,15,12,
    16,17,18, 18,19,16,
    20,21,22, 22,23,20
};

static bool key_state[1024] = { false };
static bool light_control = false;
static double last_mouse_x = 0.0, last_mouse_y = 0.0;
static bool first_mouse = true;

static vec3 cam_pos = { 0.0f, 1.0f, 5.0f };
static float yaw = -90.0f, pitch = 0.0f, fov = 60.0f;

static vec3 light_pos = { 2.0f, 2.0f, 2.0f };

void key_cb(GLFWwindow* w, int key, int sc, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, GLFW_TRUE);
    if (key >= 0 && key < 1024)
        key_state[key] = (action != GLFW_RELEASE);
    if (action == GLFW_PRESS && key == GLFW_KEY_L)
        light_control = !light_control;
}

void cursor_cb(GLFWwindow* w, double xpos, double ypos) {
    if (first_mouse) {
        last_mouse_x = xpos; last_mouse_y = ypos;
        first_mouse = false;
        return;
    }
    double dx = xpos - last_mouse_x;
    double dy = ypos - last_mouse_y;
    last_mouse_x = xpos; last_mouse_y = ypos;

    float sens = 0.12f;
    yaw += dx * sens;
    pitch -= dy * sens;
    if (pitch > 89) pitch = 89;
    if (pitch < -89) pitch = -89;
}

void mat4_from_camera(mat4x4 out, vec3 pos, float yaw_deg, float pitch_deg) {
    float yaw = yaw_deg * (float)M_PI / 180.0f;
    float pitch = pitch_deg * (float)M_PI / 180.0f;

    vec3 forward;
    forward[0] = cosf(pitch) * cosf(yaw);
    forward[1] = sinf(pitch);
    forward[2] = cosf(pitch) * sinf(yaw);

    vec3 center;
    vec3_add(center, pos, forward);

    vec3 up = { 0.0f, 1.0f, 0.0f };
    mat4x4_look_at(out, pos, center, up);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "Lighting + Materials", NULL, NULL);
    if (!win) { glfwTerminate(); return -1; }

    glfwSetKeyCallback(win, key_cb);
    glfwSetCursorPosCallback(win, cursor_cb);
    glfwMakeContextCurrent(win);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to load GL\n";
        return -1;
    }

    // deklaracja
    GLuint diffuseTex;
    int texWidth, texHeight, texChannels;
    unsigned char* data = stbi_load("C:/Users/ambro/Pictures/nig123.png", &texWidth, &texHeight, &texChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture\n";
    }

    // tworzenie tekstury w OpenGL
    glGenTextures(1, &diffuseTex);
    glBindTexture(GL_TEXTURE_2D, diffuseTex);

    // przes³anie danych do GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0,
        (texChannels == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // ustawienia filtrowania i powtarzania
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    glEnable(GL_DEPTH_TEST);

    GLuint vao, vbo, ibo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    GLsizei stride = sizeof(Vertex);

    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

    glBindVertexArray(0);

    GLuint vs1 = compile_shader_from_file("vs_diffuse.glsl", GL_VERTEX_SHADER);
    GLuint fs1 = compile_shader_from_file("fs_diffuse.glsl", GL_FRAGMENT_SHADER);
    GLuint prog1 = link_program(vs1, fs1);

    GLuint vs2 = compile_shader_from_file("vs_specular.glsl", GL_VERTEX_SHADER);
    GLuint fs2 = compile_shader_from_file("fs_specular.glsl", GL_FRAGMENT_SHADER);
    GLuint prog2 = link_program(vs2, fs2);

    GLuint vs3 = compile_shader_from_file("vs_blinnphong.glsl", GL_VERTEX_SHADER);
    GLuint fs3 = compile_shader_from_file("fs_blinnphong.glsl", GL_FRAGMENT_SHADER);
    GLuint prog3 = link_program(vs3, fs3);

    GLuint vs4 = compile_shader_from_file("vs_normalmap.glsl", GL_VERTEX_SHADER);
    GLuint fs4 = compile_shader_from_file("fs_normalmap.glsl", GL_FRAGMENT_SHADER);
    GLuint prog4 = link_program(vs4, fs4);

    if (!prog1 || !prog2 || !prog3 || !prog4) {
        std::cerr << "Shader error – exiting\n";
        return -1;
    }

    GLint uni_MVP1 = glGetUniformLocation(prog1, "MVP");
    GLint uni_lightPos1 = glGetUniformLocation(prog1, "lightPos");
    GLint uni_viewPos1 = glGetUniformLocation(prog1, "viewPos");
    GLint uni_color1 = glGetUniformLocation(prog1, "objColor");

    GLint uni_MVP2 = glGetUniformLocation(prog2, "MVP");
    GLint uni_lightPos2 = glGetUniformLocation(prog2, "lightPos");
    GLint uni_viewPos2 = glGetUniformLocation(prog2, "viewPos");
    GLint uni_specPower2 = glGetUniformLocation(prog2, "specPower");

    GLint uni_MVP3 = glGetUniformLocation(prog3, "MVP");
    GLint uni_lightPos3 = glGetUniformLocation(prog3, "lightPos");
    GLint uni_viewPos3 = glGetUniformLocation(prog3, "viewPos");
    GLint uni_ambient3 = glGetUniformLocation(prog3, "ambientFactor");

    GLint uni_MVP4 = glGetUniformLocation(prog4, "MVP");
    GLint uni_lightPos4 = glGetUniformLocation(prog4, "lightPos");
    GLint uni_viewPos4 = glGetUniformLocation(prog4, "viewPos");
    GLint uni_bumpScale4 = glGetUniformLocation(prog4, "bumpScale");

    GLuint vao_light; glGenVertexArrays(1, &vao_light);
    glBindVertexArray(vao_light);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - last_time);
        last_time = now;

        float move_speed = 4.0f;
        if (light_control) {
            if (key_state[GLFW_KEY_W]) light_pos[2] -= move_speed * dt;
            if (key_state[GLFW_KEY_S]) light_pos[2] += move_speed * dt;
            if (key_state[GLFW_KEY_A]) light_pos[0] -= move_speed * dt;
            if (key_state[GLFW_KEY_D]) light_pos[0] += move_speed * dt;
            if (key_state[GLFW_KEY_SPACE]) light_pos[1] += move_speed * dt;
            if (key_state[GLFW_KEY_C]) light_pos[1] -= move_speed * dt;
        }
        else {
            float radYaw = yaw * (float)M_PI / 180.0f;
            float radPitch = pitch * (float)M_PI / 180.0f;
            vec3 forward = { cosf(radPitch) * cosf(radYaw), sinf(radPitch), cosf(radPitch) * sinf(radYaw) };
            vec3 up = { 0,1,0 }, right;
            right[0] = forward[1] * up[2] - forward[2] * up[1];
            right[1] = forward[2] * up[0] - forward[0] * up[2];
            right[2] = forward[0] * up[1] - forward[1] * up[0];

            vec3 forward_flat = { forward[0], 0, forward[2] };
            float len = sqrtf(forward_flat[0] * forward_flat[0] + forward_flat[2] * forward_flat[2]);
            if (len > 1e-6f) { forward_flat[0] /= len; forward_flat[2] /= len; }
            float speed = 6.0f;
            if (key_state[GLFW_KEY_W]) { cam_pos[0] += forward_flat[0] * speed * dt; cam_pos[2] += forward_flat[2] * speed * dt; }
            if (key_state[GLFW_KEY_S]) { cam_pos[0] -= forward_flat[0] * speed * dt; cam_pos[2] -= forward_flat[2] * speed * dt; }
            if (key_state[GLFW_KEY_A]) { cam_pos[0] -= right[0] * speed * dt; cam_pos[2] -= right[2] * speed * dt; }
            if (key_state[GLFW_KEY_D]) { cam_pos[0] += right[0] * speed * dt; cam_pos[2] += right[2] * speed * dt; }
        }

        int w, h; glfwGetFramebufferSize(win, &w, &h);
        float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
        glViewport(0, 0, w, h);
        glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4x4 P; mat4x4_perspective(P, fov * (float)M_PI / 180.0f, aspect, 0.1f, 1000.0f);
        mat4x4 V; mat4x4_identity(V);
        mat4_from_camera(V, cam_pos, yaw, pitch);

        vec3 positions[4] = {
            { -3.0f, 1.0f, -2.0f }, // diffuse
            {  0.0f, 1.0f, -2.0f }, // specular
            {  3.0f, 1.0f, -2.0f }, // blinn-phong
            {  0.0f, 1.0f,  2.0f }  // normal mapped
        };

        // ----- 1) Diffuse-only -----
        glUseProgram(prog1);
        mat4x4 M1; mat4x4_identity(M1); mat4x4_translate(M1, positions[0][0], positions[0][1], positions[0][2]); mat4x4_scale_aniso(M1, M1, 1.2f, 1.2f, 1.2f);
        mat4x4 mvp1; mat4x4_mul(mvp1, P, V); mat4x4_mul(mvp1, mvp1, M1);
        glUniformMatrix4fv(uni_MVP1, 1, GL_FALSE, (const GLfloat*)mvp1);
        glUniform3f(uni_lightPos1, light_pos[0], light_pos[1], light_pos[2]);
        glUniform3f(uni_viewPos1, cam_pos[0], cam_pos[1], cam_pos[2]);
        glUniform3f(uni_color1, 0.8f, 0.2f, 0.2f); // red-ish
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]), GL_UNSIGNED_SHORT, 0);

        // ----- 2) Specular-only -----

        glUseProgram(prog2);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTex);
        glUniform1i(glGetUniformLocation(prog2, "diffuseMap"), 0);
        mat4x4 M2; mat4x4_identity(M2); mat4x4_translate(M2, positions[1][0], positions[1][1], positions[1][2]); mat4x4_scale_aniso(M2, M2, 1.2f, 1.2f, 1.2f);
        mat4x4 mvp2; mat4x4_mul(mvp2, P, V); mat4x4_mul(mvp2, mvp2, M2);
        glUniformMatrix4fv(uni_MVP2, 1, GL_FALSE, (const GLfloat*)mvp2);
        glUniform3f(uni_lightPos2, light_pos[0], light_pos[1], light_pos[2]);
        glUniform3f(uni_viewPos2, cam_pos[0], cam_pos[1], cam_pos[2]);
        glUniform1f(uni_specPower2, 64.0f);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]), GL_UNSIGNED_SHORT, 0);

        // ----- 3) Blinn-Phong (ambient+diffuse+specular) -----
        glUseProgram(prog3);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTex);
        glUniform1i(glGetUniformLocation(prog3, "diffuseMap"), 0);   
        mat4x4 M3; mat4x4_identity(M3); mat4x4_translate(M3, positions[2][0], positions[2][1], positions[2][2]); mat4x4_scale_aniso(M3, M3, 1.2f, 1.2f, 1.2f);
        mat4x4 mvp3; mat4x4_mul(mvp3, P, V); mat4x4_mul(mvp3, mvp3, M3);
        glUniformMatrix4fv(uni_MVP3, 1, GL_FALSE, (const GLfloat*)mvp3);
        glUniform3f(uni_lightPos3, light_pos[0], light_pos[1], light_pos[2]);
        glUniform3f(uni_viewPos3, cam_pos[0], cam_pos[1], cam_pos[2]);
        glUniform1f(uni_ambient3, 0.15f);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]), GL_UNSIGNED_SHORT, 0);

        //// ----- 4) Normal-mapped (procedural bump) -----
        glUseProgram(prog4);

        GLint uni_Model4 = glGetUniformLocation(prog4, "Model");
        GLint uni_View4 = glGetUniformLocation(prog4, "View");
        GLint uni_Projection4 = glGetUniformLocation(prog4, "Projection");

        mat4x4 M4;
        mat4x4_identity(M4);
        mat4x4_translate(M4, positions[3][0], positions[3][1], positions[3][2]);
        mat4x4_scale_aniso(M4, M4, 1.2f, 1.2f, 1.2f);
        glUniformMatrix4fv(uni_Model4, 1, GL_FALSE, (const GLfloat*)M4);
        glUniformMatrix4fv(uni_View4, 1, GL_FALSE, (const GLfloat*)V);
        glUniformMatrix4fv(uni_Projection4, 1, GL_FALSE, (const GLfloat*)P);
        glUniform3f(uni_lightPos4, light_pos[0], light_pos[1], light_pos[2]);
        glUniform3f(uni_viewPos4, cam_pos[0], cam_pos[1], cam_pos[2]);
        glUniform1f(uni_bumpScale4, 0.25f);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]), GL_UNSIGNED_SHORT, 0);


        // swiatlo
        glUseProgram(prog1);
        mat4x4 ML; mat4x4_identity(ML); mat4x4_translate(ML, light_pos[0], light_pos[1], light_pos[2]); mat4x4_scale_aniso(ML, ML, 0.15f, 0.15f, 0.15f);
        mat4x4 mvpL; mat4x4_mul(mvpL, P, V); mat4x4_mul(mvpL, mvpL, ML);
        glUniformMatrix4fv(uni_MVP1, 1, GL_FALSE, (const GLfloat*)mvpL);
        glUniform3f(uni_color1, 1.0f, 0.9f, 0.6f); 
        glBindVertexArray(vao_light);
        glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(cube_indices[0]), GL_UNSIGNED_SHORT, 0);

        static double hud_timer = 0.0; hud_timer += dt;
        if (hud_timer > 0.5) {
            hud_timer = 0.0;
            fprintf(stderr, "cam=(%.2f,%.2f,%.2f) yaw=%.1f pitch=%.1f  light=(%.2f,%.2f,%.2f) mode=%s\n",
                cam_pos[0], cam_pos[1], cam_pos[2], yaw, pitch, light_pos[0], light_pos[1], light_pos[2], light_control ? "LIGHT" : "CAMERA");
        }

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteProgram(prog1); glDeleteProgram(prog2); glDeleteProgram(prog3); glDeleteProgram(prog4);
    glDeleteShader(vs1); glDeleteShader(fs1); glDeleteShader(vs2); glDeleteShader(fs2);
    glDeleteShader(vs3); glDeleteShader(fs3); glDeleteShader(vs4); glDeleteShader(fs4);
    glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ibo); glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(win);
    glfwTerminate();

    return 0;
}
