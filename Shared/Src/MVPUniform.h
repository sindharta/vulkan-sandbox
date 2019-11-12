#pragma once

#include <glm/glm.hpp>

struct MVPUniform{
    glm::mat4 ModelMat;
    glm::mat4 ViewMat;
    glm::mat4 ProjMat;
};