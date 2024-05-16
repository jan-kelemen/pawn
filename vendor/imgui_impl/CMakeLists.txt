add_library(imgui_impl)

target_sources(imgui_impl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_sdl2.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_vulkan.h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_sdl2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_vulkan.cpp
)

target_include_directories(imgui_impl SYSTEM
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(imgui_impl
    PUBLIC
        imgui::imgui
        vulkan-headers::vulkan-headers
        SDL2::SDL2
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_vulkan.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_sdl2.cpp 
    PROPERTIES
        SKIP_LINTING ON
)
