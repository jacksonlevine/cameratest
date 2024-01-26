cmake_minimum_required(VERSION 3.12)

# set the project name
project(cameratest)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_executable(main src/main.cpp src/collcage.cpp src/glyphface.cpp src/textview.cpp src/crossmesh.cpp)
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()


target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(main PRIVATE glfw)
find_package(glm CONFIG REQUIRED)
target_link_libraries(main PRIVATE glm::glm)
find_package(GLEW REQUIRED)
target_link_libraries(main PRIVATE GLEW::GLEW)

