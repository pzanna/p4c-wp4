# Makefile for the WP4 P4-16 back-end.
# To be included in the main P4C compiler makefile

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (P4C_WP4_SOURCES
  p4c-wp4.cpp
  wp4-Backend.cpp
  wp4-Program.cpp
  wp4-Table.cpp
  wp4-Control.cpp
  wp4-Parser.cpp
  wp4-Target.cpp
  wp4-Type.cpp
  wp4-CodeGen.cpp
  wp4-Model.cpp
  wp4-Midend.cpp
  wp4-Lower.cpp
  )

set (P4C_WP4_HEADERS
  wp4-CodeGen.h
  wp4-Backend.h
  wp4-Control.h
  wp4-Model.h
  wp4-Object.h
  wp4-Program.h
  wp4-Options.h
  wp4-Parser.h
  wp4-Table.h
  wp4-Type.h
  wp4-Midend.h
  wp4-Target.h
  wp4-Lower.h
  )

set (P4C_WP4_DIST_HEADERS p4include/wp4_model.p4)

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "$(P4C_WP4_SOURCES)")

#build_unified(P4C_WP4_SOURCES ALL)
add_executable(p4c-wp4 ${P4C_WP4_SOURCES})
target_link_libraries (p4c-wp4 ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
#add_dependencies(p4c-wp4 genIR frontend)

install (TARGETS p4c-wp4
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
        DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})


add_custom_target(linkp4cwp4
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-wp4 ${P4C_BINARY_DIR}/p4c-wp4
        COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${P4C_WP4_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
        )

add_dependencies(p4c_driver linkp4cwp4)
