PROJECT(uot)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_FLAGS "-g ")

include_directories(
    ${PROJECT_SOURCE_DIR}/ev/
)

link_directories(
    ${PROJECT_SOURCE_DIR}/ev/
)

add_subdirectory(ev)

AUX_SOURCE_DIRECTORY(. SRC)
add_executable(uot ${SRC})

target_link_libraries(uot ev m)
