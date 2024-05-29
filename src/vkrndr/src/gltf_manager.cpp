#include <gltf_manager.hpp>

#include <cppext_numeric.hpp>
#include <cppext_pragma_warning.hpp>

#include <tinygltf_impl.hpp>

#include <fmt/core.h>
#include <fmt/std.h> // IWYU pragma: keep

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// IWYU pragma: no_include <map>
// IWYU pragma: no_include <glm/detail/qualifier.hpp>

namespace
{
    template<typename T>
    [[nodiscard]] constexpr size_t size_cast(T const value)
    {
        return cppext::narrow<size_t>(value);
    }

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    struct [[nodiscard]] primitive_data final
    {
        tinygltf::Accessor const& accessor;
        tinygltf::Buffer const& buffer;
        tinygltf::BufferView const& view;
    };

    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    constexpr primitive_data data_for(tinygltf::Model const& model,
        int const accessor_index)
    {
        tinygltf::Accessor const& accessor{
            model.accessors[size_cast(accessor_index)]};

        tinygltf::BufferView const& view{
            model.bufferViews[size_cast(accessor.bufferView)]};

        tinygltf::Buffer const& buffer{model.buffers[size_cast(view.buffer)]};

        return {accessor, buffer, view};
    }

    template<typename TargetType>
    struct [[nodiscard]] primitive_buffer final
    {
        TargetType const* ptr{};
        size_t stride{};
        size_t count{};

        constexpr operator std::tuple<TargetType const*&, size_t&, size_t&>()
        {
            return {ptr, stride, count};
        }
    };

    template<typename TargetType>
    constexpr primitive_buffer<TargetType>
    buffer_for(tinygltf::Model const& model, int const accessor_index)
    {
        auto const& [accessor, buffer, view] = data_for(model, accessor_index);

        primitive_buffer<TargetType> rv;

        // NOLINTNEXTLINE
        rv.ptr = reinterpret_cast<TargetType const*>(
            &(buffer.data[accessor.byteOffset + view.byteOffset]));

        size_t const byte_stride{size_cast(accessor.ByteStride(view))};

        rv.stride = byte_stride
            ? byte_stride / sizeof(float)
            : size_cast(tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));

        rv.count = accessor.count;

        return rv;
    }

    std::vector<uint32_t> load_indices(tinygltf::Model const& model,
        tinygltf::Primitive const& primitive)
    {
        auto const& [accessor, buffer, view] =
            data_for(model, primitive.indices);

        std::vector<uint32_t> rv;
        rv.reserve(accessor.count);

        // glTF supports different component types of indices
        switch (accessor.componentType)
        {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
            // NOLINTNEXTLINE
            auto const* const buf{reinterpret_cast<uint32_t const*>(
                &buffer.data[accessor.byteOffset + view.byteOffset])};
            rv.insert(std::end(rv), buf, buf + accessor.count);
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
            // NOLINTNEXTLINE
            auto const* const buf{reinterpret_cast<uint16_t const*>(
                &buffer.data[accessor.byteOffset + view.byteOffset])};
            std::ranges::copy(buf,
                buf + accessor.count,
                std::back_inserter(rv));
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
            DISABLE_WARNING_PUSH
            DISABLE_WARNING_USELESS_CAST
            // NOLINTNEXTLINE
            auto const* const buf{reinterpret_cast<uint8_t const*>(
                &buffer.data[accessor.byteOffset + view.byteOffset])};
            DISABLE_WARNING_POP

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

    void load_transform(tinygltf::Node const& node, vkrndr::gltf_node& new_node)
    {
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
        }
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
        load_transform(node, new_node);

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
                    float const* normal_buffer{nullptr};
                    size_t vertex_count{};

                    size_t position_stride{};
                    size_t normal_stride{};

                    // Get buffer data for vertex normals
                    if (auto const it{primitive.attributes.find("POSITION")};
                        it != primitive.attributes.end())
                    {
                        std::tie(position_buffer,
                            position_stride,
                            vertex_count) =
                            buffer_for<float>(model, it->second);
                    }

                    if (auto const it{primitive.attributes.find("NORMAL")};
                        it != primitive.attributes.end())
                    {
                        size_t normal_count; // NOLINT
                        std::tie(normal_buffer, normal_stride, normal_count) =
                            buffer_for<float>(model, it->second);
                    }

                    for (size_t vertex{}; vertex != vertex_count; ++vertex)
                    {
                        gltf_vertex const vert{
                            .position = glm::make_vec3(
                                &position_buffer[vertex * position_stride]),
                            .normal = glm::normalize(normal_buffer
                                    ? glm::make_vec3(&normal_buffer[vertex *
                                          normal_stride])
                                    : glm::fvec3(0.0f))};

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
