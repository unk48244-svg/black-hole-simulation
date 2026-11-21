#pragma once
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <functional>
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

// Console: ` toggles, Enter runs. DebugUI::Register("name", "help", [](const Args& a){ ... });
class DebugUI {
public:
    using Args = std::vector<std::string>;
    using Fn = std::function<void(const Args&)>;

    static void Log(const std::string& m)   { Add(m, 0); }
    static void Warn(const std::string& m)  { Add("[WARN] " + m, 1); }
    static void Error(const std::string& m) { Add("[ERR] " + m, 2); }
    static void Register(const std::string& n, const std::string& help, Fn f) { cmds[n] = { help, f }; }

    static bool Init(GLFWwindow* w) {
        ctx = nk_glfw3_init(&glfw, w, NK_GLFW3_INSTALL_CALLBACKS);
        if (!ctx) return false;
        nk_font_atlas* a;
        nk_glfw3_font_stash_begin(&glfw, &a);
        nk_glfw3_font_stash_end(&glfw);

        nk_color t[NK_COLOR_COUNT];
        for (auto& c : t) c = nk_rgb(45, 45, 48);
        t[NK_COLOR_TEXT]                    = nk_rgb(215, 215, 215);
        t[NK_COLOR_WINDOW]                  = nk_rgba(28, 28, 30, 240);
        t[NK_COLOR_BORDER]                  = nk_rgb(60, 60, 64);
        t[NK_COLOR_EDIT]                    = nk_rgb(18, 18, 20);
        t[NK_COLOR_EDIT_CURSOR]             = nk_rgb(235, 235, 235);
        t[NK_COLOR_SCROLLBAR]               = nk_rgb(18, 18, 20);
        t[NK_COLOR_SCROLLBAR_CURSOR]        = nk_rgb(75, 75, 82);
        t[NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = nk_rgb(100, 100, 110);
        t[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgb(125, 125, 138);
        nk_style_from_table(ctx, t);
        ctx->style.window.fixed_background = nk_style_item_color(t[NK_COLOR_WINDOW]);
        ctx->style.window.group_border = 0;
        ctx->style.window.rounding = 4;
        ctx->style.window.scrollbar_size = nk_vec2(8, 8);
        ctx->style.edit.rounding = 3;

        Register("help",  "list commands", [](const Args&) { for (auto& c : cmds) Log(c.first + " - " + c.second.first); });
        Register("clear", "clear console", [](const Args&) { logs.clear(); });
        return true;
    }

    static void Shutdown()   { if (ctx) { nk_glfw3_shutdown(&glfw); ctx = nullptr; } }
    static void BeginFrame() { if (ctx) nk_glfw3_new_frame(&glfw); }
    static bool WantsKeyboard() { return open; }
    static bool WantsMouse() { return open && ctx && (nk_item_is_any_active(ctx) || nk_window_is_any_hovered(ctx)); }

    static void Update(GLFWwindow* w) {
        bool k = glfwGetKey(w, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
        if (k && !was) { open = !open; focus = open; }
        was = k;
    }

    static void DrawUI() {
        if (!ctx || !open) return;
        if (nk_begin(ctx, "Console", nk_rect(10, 10, 460, 260),
                     NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_NO_SCROLLBAR)) {
            static const nk_color col[] = { {215,215,215,255}, {255,200,70,255}, {255,100,100,255}, {130,180,255,255} };
            float h = nk_window_get_content_region_size(ctx).y - 36.0f;

            nk_layout_row_dynamic(ctx, h < 30 ? 30 : h, 1);
            if (nk_group_scrolled_offset_begin(ctx, &sx, &sy, "Logs", 0)) {
                nk_layout_row_dynamic(ctx, 16, 1);
                for (auto& l : logs) nk_label_colored(ctx, l.first.c_str(), NK_TEXT_LEFT, col[l.second]);
                if (scroll) { sy = 1000000; scroll = false; }
                nk_group_scrolled_end(ctx);
            }

            nk_layout_row_dynamic(ctx, 26, 1);
            if (focus) { nk_edit_focus(ctx, 0); focus = false; }
            nk_flags ev = nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, buf, sizeof(buf), Filter);
            if ((ev & NK_EDIT_COMMITED) && buf[0]) { Run(buf); buf[0] = 0; focus = true; }
        }
        nk_end(ctx);
    }

    static void Render() {
        if (!ctx || !open) return;
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
        glEnable(GL_DEPTH_TEST);
    }

private:
    static inline nk_glfw glfw = {0};
    static inline nk_context* ctx = nullptr;
    static inline std::deque<std::pair<std::string, int>> logs;
    static inline std::map<std::string, std::pair<std::string, Fn>> cmds;
    static inline bool open = true, was = false, focus = true, scroll = false;
    static inline nk_uint sx = 0, sy = 0;
    static inline char buf[256] = {0};

    static int Filter(const nk_text_edit* b, nk_rune c) { return (c == '`' || c == '~') ? 0 : nk_filter_default(b, c); }

    static void Add(const std::string& m, int lvl) {
        logs.push_back({ m, lvl });
        if (logs.size() > 300) logs.pop_front();
        scroll = true;
    }

    static void Run(const std::string& line) {
        Add("> " + line, 3);
        std::istringstream s(line);
        std::string name, t;
        Args a;
        s >> name;
        while (s >> t) a.push_back(t);
        auto it = cmds.find(name);
        if (it == cmds.end()) Error("Unknown command: " + name);
        else it->second.second(a);
    }
};