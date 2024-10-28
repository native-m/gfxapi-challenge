cmake_minimum_required (VERSION 3.15)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("TriangleAcrossAPI")

add_subdirectory(shaders)

add_library(mainlib "main.cpp")
target_compile_definitions(mainlib PUBLIC -DUNICODE)
set_property(TARGET mainlib PROPERTY CXX_STANDARD 20)

add_executable(tri_d3d11 "tri_d3d11.cpp")
target_link_libraries(tri_d3d11 PUBLIC mainlib)
add_dependencies(tri_d3d11 d3d11_shaders)
set_property(TARGET tri_d3d11 PROPERTY CXX_STANDARD 20)

add_executable(tri_d3d12 "tri_d3d12.cpp")
target_link_libraries(tri_d3d12 PUBLIC mainlib)
set_property(TARGET tri_d3d12 PROPERTY CXX_STANDARD 20)
