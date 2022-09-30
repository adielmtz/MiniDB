cmake_minimum_required(VERSION 3.20)
project(MiniDB C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall)

add_executable(MiniDB main.c minidb.c tree.c)
