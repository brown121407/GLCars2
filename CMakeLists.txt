cmake_minimum_required(VERSION 3.23)
project(GLCars2)

set(CMAKE_CXX_STANDARD 20)

add_executable(GLCars2 src/main.cpp)

find_package(GLEW REQUIRED)
find_package(GLM REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(assimp REQUIRED)

target_link_libraries(GLCars2 glfw glm::glm GLEW::GLEW assimp::assimp)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)
add_dependencies(GLCars2 copy_assets)