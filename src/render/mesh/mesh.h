#pragma once
#include <GL/glew.h>
#include <cstddef>

// 8 floats = 32 bytes per vertex (same stride as in your setupGl).
struct Vertex {
    float pos[3];     // location = 0
    float color[3];   // location = 1
    float uv[2];      // location = 2 (for textures later)
};

class Mesh {
public:
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

    void create(const Vertex* vertices, size_t vertexCount,
                const unsigned int* indices, size_t indicesCount) {
        indexCount = (GLsizei)indicesCount;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);          // everything below is remembered by this VAO

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertexCount * sizeof(Vertex)),
                     vertices, GL_STATIC_DRAW);

        // EBO binding is stored inside the VAO, so do NOT unbind it before the VAO.
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indicesCount * sizeof(unsigned int)),
                     indices, GL_STATIC_DRAW);

        attrib(0, 3, offsetof(Vertex, pos));
        attrib(1, 3, offsetof(Vertex, color));
        attrib(2, 2, offsetof(Vertex, uv));

        glBindVertexArray(0);
    }

    // Convenience overload for plain C arrays.
    template <size_t V, size_t I>
    void create(const Vertex (&vertices)[V], const unsigned int (&indices)[I]) {
        create(vertices, V, indices, I);
    }

    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void destroy() {
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        vao = vbo = ebo = 0;
        indexCount = 0;
    }

private:
    static void attrib(GLuint index, GLint count, size_t offset) {
        glVertexAttribPointer(index, count, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offset);
        glEnableVertexAttribArray(index);
    }
};