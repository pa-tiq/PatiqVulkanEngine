#pragma once

#include <memory>

#include "pve_model.hpp"

namespace pve {
struct Transform2dComponent {
    glm::vec2 translation{};  // move objects up, down, left, right
    glm::vec2 scale{1.f, 1.f};
    float rotation;

    glm::mat2 mat2() {
        const float s = glm::sin(rotation);
        const float c = glm::cos(rotation);
        glm::mat2 rotMatrix{{c, s}, {-s, c}};
        glm::mat2 scaleMat(
            {scale.x, .0f},
            {.0f, scale.y});  // scale matrix - each {} is a column
        return rotMatrix * scaleMat;
    }
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
    Transform2dComponent transform2d{};

   private:
    PveGameObject(id_t objId) : id{objId} {}

    id_t id;
};
}  // namespace pve