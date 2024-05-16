add_library(project-options INTERFACE)

include(${CMAKE_CURRENT_LIST_DIR}/compiler-warnings.cmake)

if(PAWN_ENABLE_CLANG_FORMAT)
    include(${CMAKE_CURRENT_LIST_DIR}/clang-format.cmake)
    add_dependencies(project-options clang-format)
endif()

if(PAWN_ENABLE_COMPILER_STATIC_ANALYSIS)
    include(${CMAKE_CURRENT_LIST_DIR}/compiler-analyzer.cmake)
    target_link_libraries(project-options
        INTERFACE 
            compiler-analyzer)
endif()

