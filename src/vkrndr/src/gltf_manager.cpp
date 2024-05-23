#include <gltf_manager.hpp>

#include <tinygltf_impl.hpp>

#include <fmt/core.h>
#include <fmt/std.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

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
            tinygltf::Mesh const& mesh{model.meshes[node.mesh]};

            for (tinygltf::Primitive const& primitive : mesh.primitives)
            {
                gltf_primitive new_primitive;
                // Vertices
                {
                    float const* positionBuffer = nullptr;
                    size_t vertexCount = 0;

                    // Get buffer data for vertex normals
                    if (primitive.attributes.find("POSITION") !=
                        primitive.attributes.end())
                    {
                        tinygltf::Accessor const& accessor =
                            model.accessors
                                [primitive.attributes.find("POSITION")->second];
                        tinygltf::BufferView const& view =
                            model.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<float const*>(&(
                            model.buffers[view.buffer]
                                .data[accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }

                    for (size_t v = 0; v < vertexCount; v++)
                    {
                        gltf_vertex vert{};
                        vert.position = glm::make_vec3(&positionBuffer[v * 3]);
                        new_primitive.vertices.push_back(vert);
                    }
                }

                // Indices
                {
                    tinygltf::Accessor const& accessor =
                        model.accessors[primitive.indices];
                    tinygltf::BufferView const& bufferView =
                        model.bufferViews[accessor.bufferView];
                    tinygltf::Buffer const& buffer =
                        model.buffers[bufferView.buffer];

                    // glTF supports different component types of indices
                    switch (accessor.componentType)
                    {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    {
                        uint32_t const* buf = reinterpret_cast<uint32_t const*>(
                            &buffer.data[accessor.byteOffset +
                                bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            new_primitive.indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    {
                        uint16_t const* buf = reinterpret_cast<uint16_t const*>(
                            &buffer.data[accessor.byteOffset +
                                bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            new_primitive.indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    {
                        uint8_t const* buf = reinterpret_cast<uint8_t const*>(
                            &buffer.data[accessor.byteOffset +
                                bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            new_primitive.indices.push_back(buf[index]);
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error{
                            fmt::format("Index component type {} not supported",
                                accessor.componentType)};
                    }
                }
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
