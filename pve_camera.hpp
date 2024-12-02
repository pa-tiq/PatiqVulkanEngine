#pragma once

#define GLM_FORCE_RADIANS            // No matter what system i'm in, angles are in radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Forces GLM to expect depth buffer values to range from 0 to 1 instead of -1 to 1 (the opengl standard)
#include <glm/glm.hpp>

namespace pve {
class PveCamera {
   public:
    void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
    void setPerspectiveProjection(float fovy, float aspect, float near, float far);
    const glm::mat4& getProjection() const { return projectionMatrix; }

   private:
    glm::mat4 projectionMatrix{1.f};
};
}  // namespace pve
