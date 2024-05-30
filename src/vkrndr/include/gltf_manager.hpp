#ifndef VKRNDR_GLTF_MANAGER_INCLUDED
#define VKRNDR_GLTF_MANAGER_INCLUDED

#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp> // IWYU pragma: keep
#include <glm/mat4x4.hpp> // IWYU pragma: keep
#include <glm/vec3.hpp> // IWYU pragma: keep

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace vkrndr
{
    struct [[nodiscard]] gltf_vertex final
    {
        glm::fvec3 position;
        glm::fvec3 normal;
    };

    struct [[nodiscard]] gltf_primitive final
    {
        std::vector<gltf_vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct [[nodiscard]] gltf_mesh final
    {
        std::vector<gltf_primitive> primitives;
    };

    struct [[nodiscard]] gltf_node final
    {
        std::string name;
        std::optional<gltf_mesh> mesh;
        glm::fvec3 translation{0.0f};
        glm::fvec3 scale{1.0f};
        glm::fquat rotation{};
        glm::fmat4 matrix{1.0f};
    };

    struct [[nodiscard]] gltf_model final
    {
        std::vector<gltf_node> nodes;
    };

    glm::fmat4 local_matrix(gltf_node const& node);

    class [[nodiscard]] gltf_manager final
    {
    public:
        gltf_model load(std::filesystem::path const& path);
    };
} // namespace vkrndr

#endif
