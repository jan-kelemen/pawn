#ifndef PAWN_SCENE_INLCUDED
#define PAWN_SCENE_INLCUDED

#include <vulkan_buffer.hpp>
#include <vulkan_image.hpp>
#include <vulkan_scene.hpp>

#include <cppext_pragma_warning.hpp>

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp> // IWYU pragma: keep
#include <glm/vec3.hpp> // IWYU pragma: keep

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace vkrndr
{
    struct vulkan_device;
    struct vulkan_pipeline;
    class vulkan_renderer;
} // namespace vkrndr

namespace pawn
{
    class uci_engine;
} // namespace pawn

namespace pawn
{
    class [[nodiscard]] orthographic_camera final
    {
    public:
        [[nodiscard]] glm::fvec3 position() const;

        [[nodiscard]] glm::fmat4 view_matrix() const;

        [[nodiscard]] glm::fmat4 projection_matrix() const;

        [[nodiscard]] float near_plane() const;

        [[nodiscard]] float far_plane() const;

    private:
        glm::fvec3 front_face_{20.f, -20.f, 20.f};
        glm::fvec3 up_direction_{0, -1, 0};
        glm::fvec3 position_{0, 0, 0};
        std::array<float, 3> projection_{10.f, -8.0f, 8.0f};
    };

    enum class piece_type : uint8_t
    {
        none,
        rook,
        knight,
        bishop,
        queen,
        king,
        pawn
    };

    struct [[nodiscard]] board_piece final
    {
        uint8_t row : 3;
        uint8_t column : 3;
        uint8_t color : 2;
        piece_type type;
        bool highlighted;
    };

    enum class mesh_color : uint8_t
    {
        none,
        white,
        black
    };

    [[nodiscard]] constexpr board_piece to_board_peice(uint8_t const row,
        uint8_t const column,
        mesh_color const color,
        piece_type const type,
        bool const highlighted)
    {
        DISABLE_WARNING_PUSH
        DISABLE_WARNING_CONVERSION
        return {row, column, std::to_underlying(color), type, highlighted};
        DISABLE_WARNING_POP
    }

    struct [[nodiscard]] mesh final
    {
        piece_type type{};
        int32_t vertex_offset{};
        uint32_t vertex_count{};
        int32_t index_offset{};
        uint32_t index_count{};
        glm::fmat4 local_matrix{};
    };

    class [[nodiscard]] scene final : public vkrndr::vulkan_scene
    {
    public: // Construction
        explicit scene(uci_engine const& engine);

        scene(scene const&) = delete;

        scene(scene&&) noexcept = delete;

    public: // Destruction
        ~scene() override;

    public: // Interface
        void attach_renderer(vkrndr::vulkan_device* device,
            vkrndr::vulkan_renderer* renderer);

        void detach_renderer();

        void begin_frame();

        void end_frame();

        void add_piece(board_piece piece);

        void update(orthographic_camera const& camera);

    public: // vulkan_scene overrides
        [[nodiscard]] VkClearValue clear_color() override;

        [[nodiscard]] VkClearValue clear_depth() override;

        [[nodiscard]] vkrndr::vulkan_image* depth_image() override;

        void resize(VkExtent2D extent) override;

        void draw(VkCommandBuffer command_buffer, VkExtent2D extent) override;

        void draw_imgui() override;

    public: // Operators
        scene& operator=(scene const&) = delete;

        scene& operator=(scene&&) noexcept = delete;

    private: // Types
        struct [[nodiscard]] frame_data final
        {
            VkDescriptorSet descriptor_set_{VK_NULL_HANDLE};
        };

    private: // Data
        uci_engine const* engine_{};

        vkrndr::vulkan_device* vulkan_device_{};
        vkrndr::vulkan_renderer* vulkan_renderer_{};

        vkrndr::vulkan_buffer vert_index_buffer_;
        vkrndr::vulkan_buffer vertex_uniform_buffer_;
        vkrndr::vulkan_image depth_buffer_;

        VkDescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};
        std::unique_ptr<vkrndr::vulkan_pipeline> piece_pipeline_;
        std::unique_ptr<vkrndr::vulkan_pipeline> outlined_piece_pipeline_;
        std::unique_ptr<vkrndr::vulkan_pipeline> outline_pipeline_;

        VkSampler texture_sampler_{VK_NULL_HANDLE};
        vkrndr::vulkan_image texture_image_;

        std::vector<frame_data> frame_data_;

        size_t vertex_count_{};
        size_t index_count_{};
        std::vector<mesh> meshes_;

        uint32_t current_frame_{};

        glm::fvec3 camera_position_{0.0f, 0.0f, 0.0f};
        glm::fvec3 light_position_{-20.0f, 20.0f, 20.0f};
        glm::fvec3 light_color_{0.8f, 0.8f, 0.8f};

        uint8_t used_pieces_{};
        std::array<board_piece, 64 + 1> draw_meshes_{};
    };
} // namespace pawn

#endif
