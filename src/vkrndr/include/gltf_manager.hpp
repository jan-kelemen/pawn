#ifndef VKRNDR_GLTF_MANAGER_INCLUDED
#define VKRNDR_GLTF_MANAGER_INCLUDED

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace vkrndr
{
    struct [[nodiscard]] gltf_vertex final
    {
        glm::fvec3 position;
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
        std::optional<gltf_mesh> mesh;
    };

    struct [[nodiscard]] gltf_model final
    {
        std::vector<gltf_node> nodes;
    };

    class [[nodiscard]] gltf_manager final
    {
    public:
        gltf_model load(std::filesystem::path const& path);
    };
} // namespace vkrndr

#endif
