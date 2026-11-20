#pragma once
// NOTE: glew.h must be included before glfw3.h (this header does it for you
// as long as it is included before window.h).
#include <GL\glew.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "Rprm.h"

class Shader {
public:
    GLuint id = 0;

    // Loads GLSL from two files (paths are relative to the working directory),
    // e.g. buildFromFiles("src/shaders/basic.vert", "src/shaders/basic.frag").
    bool buildFromFiles(const char* vertPath, const char* fragPath) {
        std::string vert, frag;
        if (!readFile(vertPath, vert) || !readFile(fragPath, frag))
            return false;
        return build(vert.c_str(), frag.c_str());
    }

    // Compiles and links a program from GLSL source strings.
    // Returns false and prints the log to stderr on error.
    bool build(const char* vertSrc, const char* fragSrc) {
        GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
        if (!vs || !fs) {
            glDeleteShader(vs);   // deleting 0 is silently ignored
            glDeleteShader(fs);
            return false;
        }

        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint ok = 0;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(id, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader link error:\n%s\n", log);
            glDeleteProgram(id);
            id = 0;
            return false;
        }
        return true;
    }

    void use() const { glUseProgram(id); }

    void destroy() {
        if (id) glDeleteProgram(id);
        id = 0;
    }

    // ---- uniforms (the program must be in use) ----
    void setMat4(const char* name, const Mat4& m) const {
        float f[16];
        m.toFloat(f);                                   // double -> float
        glUniformMatrix4fv(loc(name), 1, GL_FALSE, f);  // our Mat4 is column-major, no transpose
    }
    void setVec3(const char* name, Vec3 v) const {
        glUniform3f(loc(name), (float)v.x, (float)v.y, (float)v.z);
    }
    void setFloat(const char* name, double v) const {
        glUniform1f(loc(name), (float)v);
    }
    void setInt(const char* name, int v) const {
        glUniform1i(loc(name), v);
    }

private:
    GLint loc(const char* name) const { return glGetUniformLocation(id, name); }

    static bool readFile(const char* path, std::string& out) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::fprintf(stderr, "Cannot open shader file: %s\n", path);
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        out = ss.str();
        return true;
    }

    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);

        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            std::fprintf(stderr, "%s shader compile error:\n%s\n",
                         type == GL_VERTEX_SHADER ? "Vertex" : "Fragment", log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    }
};