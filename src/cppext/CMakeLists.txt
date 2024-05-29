add_library(cppext INTERFACE)

target_sources(cppext
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cppext_numeric.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cppext_pragma_warning.hpp
)

target_include_directories(cppext
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cppext
    INTERFACE
        project-options
)
