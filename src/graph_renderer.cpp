/* This file is part of BlosSOM.
 *
 * Copyright (C) 2021 Mirek Kratochvil
 *                    Sona Molnarova
 *
 * BlosSOM is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * BlosSOM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BlosSOM. If not, see <https://www.gnu.org/licenses/>.
 */

#include "graph_renderer.h"

#include "glm/gtc/matrix_transform.hpp"

#include <cmath>
#include <iostream>

#include "shaders.h"

GraphRenderer::GraphRenderer() {}

void
GraphRenderer::init()
{
    glGenVertexArrays(1, &VAO_v);
    glGenBuffers(1, &VBO_v);

    shader_v.build(graph_v_vs, graph_v_fs);

    glGenVertexArrays(1, &VAO_e);
    glGenBuffers(1, &VBO_e);

    shader_e.build(graph_e_vs, graph_e_fs);
}

void
GraphRenderer::draw(const View &view, const LandmarkModel &model)
{
    glEnable(GL_BLEND);

    prepare_data(view.current_zoom, model);

    shader_e.use();
    shader_e.set_mat4("model", glm::mat4(1.0f));
    shader_e.set_mat4("view", view.get_view_matrix());
    shader_e.set_mat4("proj", view.get_proj_matrix());

    glBindVertexArray(VAO_e);
    glDrawArrays(GL_LINES, 0, 2 * model.edges.size());

    shader_v.use();
    shader_v.set_mat4("model", glm::mat4(1.0f));
    shader_v.set_mat4("view", view.get_view_matrix());
    shader_v.set_mat4("proj", view.get_proj_matrix());

    glBindVertexArray(VAO_v);

    for (size_t i = 0; i < model.lodim_vertices.size(); ++i) {
        glDrawArrays(GL_TRIANGLE_FAN, i * num_all_vtxs, num_all_vtxs);
    }

    glDisable(GL_BLEND);
}

void
GraphRenderer::prepare_data(float current_zoom, const LandmarkModel &model)
{
    prepare_vertices(current_zoom, model);
    prepare_edges(model);
}

void
GraphRenderer::add_circle(float middle_x,
                          float middle_y,
                          float zoom,
                          std::vector<float> &all_vtxs)
{
    int sides = 12;
    float radius = 0.05f;
    num_all_vtxs = sides + 2;

    double two_pi = 2.0f * M_PI;

    all_vtxs.emplace_back(middle_x);
    all_vtxs.emplace_back(middle_y);

    for (int i = 1; i < num_all_vtxs; i++) {
        all_vtxs.emplace_back(middle_x +
                              (radius * cos(i * two_pi / sides)) * zoom * 130);
        all_vtxs.emplace_back(middle_y +
                              (radius * sin(i * two_pi / sides)) * zoom * 130);
    }
}

void
GraphRenderer::prepare_vertices(float current_zoom, const LandmarkModel &model)
{
    if (vertices.size() != model.lodim_vertices.size()) {
        vertices.clear();
        vertices.resize(model.lodim_vertices.size());
    }

    std::vector<float> all_vtxs;

    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i] = model.lodim_vertices[i];
        add_circle(vertices[i].x, vertices[i].y, current_zoom, all_vtxs);
    }

    glBindVertexArray(VAO_v);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_v);
    glBufferData(GL_ARRAY_BUFFER,
                 all_vtxs.size() * sizeof(float),
                 &all_vtxs[0],
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
}

void
GraphRenderer::prepare_edges(const LandmarkModel &model)
{
    std::vector<glm::vec2> edge_lines(2 * model.edges.size());
    for (size_t i = 0; i < model.edges.size(); ++i) {
        edge_lines[2 * i + 0] = vertices[model.edges[i].first];
        edge_lines[2 * i + 1] = vertices[model.edges[i].second];
    }

    glBindVertexArray(VAO_e);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_e);
    glBufferData(GL_ARRAY_BUFFER,
                 edge_lines.size() * sizeof(glm::vec2),
                 &edge_lines[0],
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
    glEnableVertexAttribArray(0);
}

bool
GraphRenderer::is_vert_pressed(const View &view,
                               glm::vec2 mouse,
                               size_t &vert_ind) const
{
    float radius = vertex_size;

    for (size_t i = 0; i < vertices.size(); ++i) {
        glm::vec2 vert = view.screen_coords(vertices[i]);

        if ((mouse.x >= roundf(vert.x) - radius) &&
            (mouse.x <= roundf(vert.x) + radius) &&
            (mouse.y >= roundf(vert.y) - radius) &&
            (mouse.y <= roundf(vert.y) + radius)) {
            vert_ind = i;
            return true;
        }
    }

    return false;
}
