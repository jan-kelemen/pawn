add_executable(pawn)

target_sources(pawn
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pawn.m.cpp
)

target_include_directories(pawn
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pawn
    PRIVATE
        glm::glm
        SDL2::SDL2main
        spdlog::spdlog
        vkrndr
        imgui_impl
        project-options
)

set_property(TARGET pawn 
    PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
