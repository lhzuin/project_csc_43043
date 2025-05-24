#pragma once


#include "cgp/cgp.hpp"   // brings in cgp::mesh

/* Add JOINTS_0 / WEIGHTS_0 attributes to the VAO created by CGP */
inline void add_skin_attributes(cgp::mesh_drawable& md,
    const cgp::numarray<cgp::uint4>& joints,
    const cgp::numarray<cgp::vec4 >& weights)
{
    GLuint vao = md.vao;
    glBindVertexArray(vao);

    /* --- JOINTS -------------------------------------------------- */
    GLuint vboJ;  glGenBuffers(1, &vboJ);
    glBindBuffer(GL_ARRAY_BUFFER, vboJ);
    glBufferData(GL_ARRAY_BUFFER,
        joints.size()*sizeof(cgp::uint4),
        joints.data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(cgp::uint4), (void*)0);

    /* --- WEIGHTS ------------------------------------------------- */
    GLuint vboW;  glGenBuffers(1, &vboW);
    glBindBuffer(GL_ARRAY_BUFFER, vboW);
    glBufferData(GL_ARRAY_BUFFER,
        weights.size()*sizeof(cgp::vec4),
        weights.data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE,
    sizeof(cgp::vec4), (void*)0);

    glBindVertexArray(0);
}
