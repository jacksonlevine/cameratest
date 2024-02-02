cmake_minimum_required(VERSION 3.12)

# set the project name
project(mimodo)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

add_executable(main src/main.cpp src/collcage.cpp src/glyphface.cpp src/textview.cpp src/crossmesh.cpp src/textureface.cpp src/guielement.cpp src/inventory.cpp)
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()


target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}/include)

  find_package(portaudio CONFIG REQUIRED)
  target_link_libraries(main PRIVATE portaudio_static)
  find_package(SndFile CONFIG REQUIRED)
  target_link_libraries(main PRIVATE SndFile::sndfile)


find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(main PRIVATE glfw)
find_package(glm CONFIG REQUIRED)
target_link_libraries(main PRIVATE glm::glm)
find_package(GLEW REQUIRED)
target_link_libraries(main PRIVATE GLEW::GLEW)

