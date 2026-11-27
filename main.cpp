#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Rprm.h"
#include "src/render/window.h"
#include "src/render/mesh/mesh.h"
#include "src/render/shader.h"
#include "src/render/camera.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION

#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

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

	struct nk_glfw glfw = {0};
	struct nk_context *nk = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);

	struct nk_font_atlas *atlas;
	nk_glfw3_font_stash_begin(&glfw, &atlas);
	nk_glfw3_font_stash_end(&glfw);

	SDC console;
	sdc_init(&console, 128);
	sdc_log(&console, SDC_INFO, "Simulation started");

	const Vertex vertices[] = {
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

		{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	};
	const unsigned int indices[] = {
		 0,  1,  2,   2,  3,  0,
		 4,  5,  6,   6,  7,  4,
		 8,  9, 10,  10, 11,  8,
		12, 13, 14,  14, 15, 12,
		16, 17, 18,  18, 19, 16,
		20, 21, 22,  22, 23, 20,
	};

	Mesh cube;
	cube.create(vertices, indices);

	Shader shader;
	if (!shader.buildFromFiles("src/shaders/basic.vert", "src/shaders/basic.frag"))
		return -1;

	Camera camera;
	camera.position = Vec3(0, 0, 3);

	unsigned long long tick = 0;
	double prev_time = glfwGetTime();
	double sim_time  = 0.0;
	bool   grave_was_down = false;

	while (!glfwWindowShouldClose(win))
	{
		double now = glfwGetTime();
		double dt  = now - prev_time;
		prev_time  = now;
		double fps = (dt > 0.0) ? 1.0 / dt : 0.0;

		bool grave_down = glfwGetKey(win, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
		if (grave_down && !grave_was_down) sdc_toggle(&console);
		grave_was_down = grave_down;
		nk_glfw3_new_frame(&glfw);

		if (!sdc_paused(&console)) {
			tick++;
			sim_time += dt;
		} else if (sdc_step(&console)) {
			tick++;
			sim_time += 1.0 / 60.0;
		}

		sdc_tick(&console, sim_time, fps, 1, tick);

		int w = 0, h = 0;
		glfwGetFramebufferSize(win, &w, &h);

		sdc_draw(&console, nk, 10.0f, 10.0f, 420.0f, (float)h - 20.0f);

		if (w == 0 || h == 0) { glfwPollEvents(); continue; }
		glViewport(0, 0, w, h);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			Mat4 model = rotate(Vec3(1, 1, 1), sim_time);
			Mat4 mvp   = camera.projection((double)w / h) * camera.view() * model;
			shader.use();
			shader.setMat4("uMVP", mvp);
			cube.draw();
		}

		nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, NK_MAX_VERTEX_BUFFER, NK_MAX_ELEMENT_BUFFER);

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	sdc_free(&console);
	cube.destroy();
	shader.destroy();
	nk_glfw3_shutdown(&glfw);
	glfwDestroyWindow(win);
	glfwTerminate();
	return 0;
}