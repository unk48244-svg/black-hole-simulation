#pragma once

#include <vector>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

class DebugUI {
public:
    static void Log(const std::string& msg) { Add(msg, 0); }
    static void Warn(const std::string& msg) { Add("[WARN] " + msg, 1); }
    static void Error(const std::string& msg) { Add("[ERR] " + msg, 2); }

    static bool Init(GLFWwindow* window) {
        ctx = nk_glfw3_init(&glfw, window, NK_GLFW3_INSTALL_CALLBACKS);
        if (!ctx) return false;
        struct nk_font_atlas* atlas;
        nk_glfw3_font_stash_begin(&glfw, &atlas);
        nk_glfw3_font_stash_end(&glfw);
        return true;
    }

    static void Shutdown() {
        if (ctx) { nk_glfw3_shutdown(&glfw); ctx = nullptr; }
    }

    static void BeginFrame() {
        if (ctx) nk_glfw3_new_frame(&glfw);
    }

    static bool WantsMouse() {
        return isOpen && ctx && (nk_item_is_any_active(ctx) || nk_window_is_any_hovered(ctx));
    }

    static bool WantsKeyboard() {
        return isOpen;
    }

    static void Update(GLFWwindow* window) {
        bool now = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
        if (now && !wasPressed) isOpen = !isOpen;
        wasPressed = now;
    }

    static void DrawUI() {
        if (!ctx || !isOpen) return;

        if (nk_begin(ctx, "Console", nk_rect(10, 10, 500, 250),
            NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE))
        {
            float h = nk_window_get_content_region_size(ctx).y - 38.0f;
            if (h < 30.0f) h = 30.0f;

            nk_layout_row_dynamic(ctx, h, 1);
            if (nk_group_scrolled_offset_begin(ctx, &sx, &sy, "Logs", NK_WINDOW_BORDER)) {
                for (const auto& item : logs) {
                    nk_layout_row_dynamic(ctx, 18, 1);
                    struct nk_color col = nk_rgb(220, 220, 220);
                    if (item.second == 1) col = nk_rgb(255, 200, 60);
                    if (item.second == 2) col = nk_rgb(255, 80, 80);
                    nk_label_colored(ctx, item.first.c_str(), NK_TEXT_LEFT, col);
                }
                if (scrollDown) { sy = 1000000; scrollDown = false; }
                nk_group_scrolled_end(ctx);
            }

            static char buf[256] = {0};
            nk_layout_row_begin(ctx, NK_DYNAMIC, 26, 2);
            nk_layout_row_push(ctx, 0.85f);
            nk_flags ev = nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, buf, sizeof(buf), nk_filter_default);
            nk_layout_row_push(ctx, 0.15f);
            if (nk_button_label(ctx, "Run") || (ev & NK_EDIT_COMMITED)) {
                if (buf[0] != '\0') {
                    RunCommand(buf);
                    buf[0] = '\0';
                }
            }
            nk_layout_row_end(ctx);
        }
        nk_end(ctx);
    }

    static void Render() {
        if (!ctx || !isOpen) return;
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        glEnable(GL_DEPTH_TEST);
    }

private:
    static inline struct nk_glfw glfw = {0};
    static inline struct nk_context* ctx = nullptr;
    static inline std::vector<std::pair<std::string, int>> logs;
    static inline bool isOpen = true;
    static inline bool wasPressed = false;
    static inline nk_uint sx = 0, sy = 0;
    static inline bool scrollDown = false;

    static void Add(const std::string& msg, int lvl) {
        logs.push_back({ msg, lvl });
        if (logs.size() > 200) logs.erase(logs.begin());
        scrollDown = true;
    }

    static void RunCommand(const std::string& cmd) {
        Log("> " + cmd);
        if (cmd == "help") {
            Log("Commands: help, clear");
        } else if (cmd == "clear") {
            logs.clear();
        } else {
            Error("Unknown command: " + cmd);
        }
    }
};