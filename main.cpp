// 1. Сначала GLEW, GLFW и базовые заголовки рендера
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Rprm.h"
#include "src/render/window.h"
#include "src/render/mesh.h"
#include "src/render/shader.h"
#include "src/render/camera.h"

// 2. Макросы Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION

// 3. Подключаем Nuklear
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

// 4. Подключаем консоль отладки с ее точным макросом реализации
#define SPACE_DEBUG_CONSOLE_IMPLEMENTATION
#include "debug_console.h"

#define NK_MAX_VERTEX_BUFFER  (512 * 1024)
#define NK_MAX_ELEMENT_BUFFER (128 * 1024)

int main()
{
    GLFWwindow* win = window::createWindow();
    if (!win) return -1;

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);

    // ---- Nuklear init ----
    struct nk_glfw glfw = {0};
    struct nk_context *nk = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);

    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    // ---- Debug console ----
    SDC console;
    sdc_init(&console, 128);
    sdc_log(&console, SDC_INFO, "Simulation started");

    // ---- Scene ----
    const Vertex vertices[] = {
        {{-0.6f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.6f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.0f,  0.6f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
    };
    const unsigned int indices[] = {0, 1, 2};

    Mesh triangle;
    triangle.create(vertices, indices);

    Shader shader;
    if (!shader.buildFromFiles("src/shaders/basic.vert", "src/shaders/basic.frag"))
        return -1;

    Camera camera;
    camera.position = Vec3(0, 0, 3);

    unsigned long long tick = 0;
    double prev_time = glfwGetTime();

    while (!glfwWindowShouldClose(win))
    {
        double now = glfwGetTime();
        double dt  = now - prev_time;
        prev_time  = now;
        double fps = (dt > 0.0) ? 1.0 / dt : 0.0;

        // ---- Nuklear input ----
        nk_glfw3_new_frame(&glfw);

        // ---- Sim tick ----
        if (!sdc_paused(&console)) {
            tick++;
        } else if (sdc_step(&console)) {
            tick++;
        }

        sdc_tick(&console, now, fps, 1, tick);

        // ---- Draw console UI ----
        {
            int w = 0, h = 0;
            glfwGetFramebufferSize(win, &w, &h);
            sdc_draw(&console, nk, 10.0f, 10.0f, 420.0f, (float)h - 20.0f);
        }

        // ---- 3D render ----
        {
            int w = 0, h = 0;
            glfwGetFramebufferSize(win, &w, &h);
            if (w == 0 || h == 0) { glfwPollEvents(); continue; }
            glViewport(0, 0, w, h);
        }

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!sdc_paused(&console)) {
            int w = 0, h = 0;
            glfwGetFramebufferSize(win, &w, &h);
            Mat4 model = rotate(Vec3(0, 1, 0), now);
            Mat4 mvp   = camera.projection((double)w / h) * camera.view() * model;
            shader.use();
            shader.setMat4("uMVP", mvp);
            triangle.draw();
        }

        // ---- Nuklear render ----
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, NK_MAX_VERTEX_BUFFER, NK_MAX_ELEMENT_BUFFER);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    sdc_free(&console);
    triangle.destroy();
    shader.destroy();
    nk_glfw3_shutdown(&glfw);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
