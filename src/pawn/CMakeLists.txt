add_executable(pawn)

target_sources(pawn
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pawn.m.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/uci_engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/uci_engine.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/uci_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/uci_parser.hpp
)

target_include_directories(pawn
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pawn
    PRIVATE
        cppext
        imgui_impl
        vkrndr
    PRIVATE
        boost::boost
        glm::glm
        SDL2::SDL2main
        spdlog::spdlog
    PRIVATE
        project-options
)
add_dependencies(pawn shaders)

add_custom_target(shaders
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/scene.frag.spv
        ${CMAKE_CURRENT_BINARY_DIR}/scene.vert.spv
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/scene.vert.spv
    COMMAND 
        ${GLSLC_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene.vert -o ${CMAKE_CURRENT_BINARY_DIR}/scene.vert.spv
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene.vert
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/scene.frag.spv
    COMMAND 
        ${GLSLC_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene.frag -o ${CMAKE_CURRENT_BINARY_DIR}/scene.frag.spv
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene.frag
)

set_property(TARGET pawn 
    PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if (PAWN_BUILD_TESTS)
    add_executable(pawn_test)

    target_sources(pawn_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/test/uci_parser.t.cpp
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/uci_parser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/uci_parser.hpp
    )

    target_compile_definitions(pawn_test PRIVATE BOOST_SPIRIT_X3_DEBUG)

    target_include_directories(pawn_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )


    target_link_libraries(pawn_test
        PRIVATE
            boost::boost
            Catch2::Catch2WithMain
            fmt::fmt
            project-options
    )

    if (NOT CMAKE_CROSSCOMPILING)
        include(Catch)
        catch_discover_tests(pawn_test)
    endif()
endif()