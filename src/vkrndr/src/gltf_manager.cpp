#include <gltf_manager.hpp>

#include <tinygltf_impl.hpp>

#include <fmt/core.h>
#include <fmt/std.h> // IWYU pragma: keep

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// IWYU pragma: no_include <map>

namespace
{
    template<typename T>
    [[nodiscard]] constexpr size_t size_cast(T const count)
    {
        assert(std::in_range<size_t>(count));
        return static_cast<size_t>(count);
    }

    std::vector<uint32_t> load_indices(tinygltf::Model const& model,
        tinygltf::Primitive const& primitive)
    {
        tinygltf::Accessor const& accessor{
            model.accessors[size_cast(primitive.indices)]};
        tinygltf::BufferView const& bufferView{
            model.bufferViews[size_cast(accessor.bufferView)]};
        tinygltf::Buffer const& buffer{
            model.buffers[size_cast(bufferView.buffer)]};

        std::vector<uint32_t> rv;
        rv.reserve(accessor.count);

        // glTF supports different component types of indices
        switch (accessor.componentType)
        {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
            // NOLINTNEXTLINE
            auto const* const buf{reinterpret_cast<uint32_t const*>(
                &buffer.data[accessor.byteOffset + bufferView.byteOffset])};
            rv.insert(std::end(rv), buf, buf + accessor.count);
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
            // NOLINTNEXTLINE
            auto const* const buf{reinterpret_cast<uint16_t const*>(
                &buffer.data[accessor.byteOffset + bufferView.byteOffset])};
            std::ranges::copy(buf,
                buf + accessor.count,
                std::back_inserter(rv));
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
            // NOLINTNEXTLINE
            auto const* const buf{reinterpret_cast<uint8_t const*>(
                &buffer.data[accessor.byteOffset + bufferView.byteOffset])};
            std::ranges::copy(buf,
                buf + accessor.count,
                std::back_inserter(rv));
            break;
        }
        default:
            throw std::runtime_error{
                fmt::format("Index component type {} not supported",
                    accessor.componentType)};
        }

        return rv;
    }
} // namespace

vkrndr::gltf_model vkrndr::gltf_manager::load(std::filesystem::path const& path)
{
    tinygltf::TinyGLTF context;

    tinygltf::Model model;
    std::string error;
    std::string warning;
    bool const loaded{path.extension() == ".gltf"
            ? context.LoadASCIIFromFile(&model, &error, &warning, path.string())
            : context.LoadBinaryFromFile(&model,
                  &error,
                  &warning,
                  path.string())};
    if (!loaded)
    {
        throw std::runtime_error{
            fmt::format("Model at path {} not loaded", path)};
    }

    gltf_model rv;
    for (tinygltf::Node const& node : model.nodes)
    {
        gltf_node new_node;
        if (node.translation.size() == 3)
        {
            new_node.translation = glm::make_vec3(node.translation.data());
        }

        if (node.rotation.size() == 4)
        {
            glm::fquat const quaternion{glm::make_quat(node.rotation.data())};
            new_node.rotation = glm::mat4{quaternion};
        }

        if (node.scale.size() == 3)
        {
            new_node.scale = glm::make_vec3(node.scale.data());
        }

        if (node.matrix.size() == 16)
        {
            new_node.matrix = glm::make_mat4x4(node.matrix.data());
        };

        if (node.mesh != -1)
        {
            gltf_mesh new_mesh;
            tinygltf::Mesh const& mesh{model.meshes[size_cast(node.mesh)]};

            for (tinygltf::Primitive const& primitive : mesh.primitives)
            {
                gltf_primitive new_primitive;

                // Vertices
                {
                    float const* position_buffer{nullptr};
                    size_t vertex_count{};

                    // Get buffer data for vertex normals
                    if (auto const it{primitive.attributes.find("POSITION")};
                        it != primitive.attributes.end())
                    {
                        tinygltf::Accessor const& accessor =
                            model.accessors[size_cast(it->second)];
                        tinygltf::BufferView const& view =
                            model.bufferViews[size_cast(accessor.bufferView)];
                        // NOLINTNEXTLINE
                        position_buffer = reinterpret_cast<float const*>(&(
                            model.buffers[size_cast(view.buffer)]
                                .data[accessor.byteOffset + view.byteOffset]));
                        vertex_count = accessor.count;
                    }

                    for (size_t vertex{}; vertex != vertex_count; ++vertex)
                    {
                        gltf_vertex const vert{
                            .position =
                                glm::make_vec3(&position_buffer[vertex * 3])};
                        static_assert(std::is_trivial_v<gltf_vertex>);
                        new_primitive.vertices.push_back(vert);
                    }
                }

                new_primitive.indices = load_indices(model, primitive);

                new_mesh.primitives.push_back(std::move(new_primitive));
            }
            new_node.mesh = std::move(new_mesh);
        }
        rv.nodes.push_back(std::move(new_node));
    }
    return rv;
}

glm::fmat4 vkrndr::local_matrix(gltf_node const& node)
{
    return glm::translate(glm::mat4(1.0f), node.translation) *
        glm::mat4(node.rotation) * glm::scale(glm::mat4(1.0f), node.scale) *
        node.matrix;
}
