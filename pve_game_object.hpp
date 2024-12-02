#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "pve_model.hpp"

namespace pve {
struct TransformComponent {
    glm::vec3 translation{};  // move objects up, down, left, right
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 rotation{};

    // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
    // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
    // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
    glm::mat4 mat4() {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        return glm::mat4{
            {
                scale.x * (c1 * c3 + s1 * s2 * s3),
                scale.x * (c2 * s3),
                scale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f,
            },
            {
                scale.y * (c3 * s1 * s2 - c1 * s3),
                scale.y * (c2 * c3),
                scale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f,
            },
            {
                scale.z * (c2 * s1),
                scale.z * (-s2),
                scale.z * (c1 * c2),
                0.0f,
            },
            {translation.x, translation.y, translation.z, 1.0f}};
    }
};

struct RigidBody2dComponent {
    glm::vec2 velocity;
    float mass{1.0f};
};

// a Game Object is anything in the game with a collection of properties and
// methods
class PveGameObject {
   public:
    using id_t = unsigned int;

    static PveGameObject createGameObject() {
        static id_t currentId = 0;
        return PveGameObject{currentId++};
    }

    // The PveGameObject class uses a unique ID (id_t) to identify objects.
    // Allowing copies or assignments could result in two objects sharing the
    // same id, breaking the uniqueness constraint. Deleting these methods
    // ensures that each PveGameObject remains distinct.

    // this deletes the copy constructor.
    // prevents creating a new PveGameObject by copying an existing one.
    // Ex: PveGameObject obj1 = PveGameObject::createGameObject();
    //     PveGameObject obj2 = obj1; // Error: copy constructor is deleted
    PveGameObject(const PveGameObject &) = delete;

    // This deletes the copy assignment operator.
    // Prevents assigning one PveGameObject to another.
    // Ex: PveGameObject obj1 = PveGameObject::createGameObject();
    //     PveGameObject obj2 = PveGameObject::createGameObject();
    //     obj2 = obj1; // Error: copy assignment operator is deleted
    PveGameObject &operator=(const PveGameObject &) = delete;

    // Moving avoids the cost of copying and ensures the uniqueness of the id is
    // maintained. The default move constructor and assignment operator are
    // sufficient since the class manages no complex resources like dynamic
    // memory or file handles.

    // This uses the default move constructor. Allows moving an object instead
    // of copying it. Moving transfers ownership of the resource (here, the ID)
    // from the source to the destination without duplication. Ex: PveGameObject
    // obj1 = PveGameObject::createGameObject();
    //     PveGameObject obj2 = std::move(obj1); // Move constructor
    PveGameObject(PveGameObject &&) = default;

    // This uses the default move assignment operator. Allows moving an object
    // during assignment. Similar to the move constructor, it transfers
    // ownership without creating a duplicate. Ex: PveGameObject obj1 =
    // PveGameObject::createGameObject();
    //     PveGameObject obj2 = PveGameObject::createGameObject();
    //     obj2 = std::move(obj1); // Move assignment operator
    PveGameObject &operator=(PveGameObject &&) = default;

    const id_t getId() { return id; }

    std::shared_ptr<PveModel> model{};
    glm::vec3 color{};
    TransformComponent transform{};

   private:
    PveGameObject(id_t objId) : id{objId} {}

    id_t id;
};
}  // namespace pve