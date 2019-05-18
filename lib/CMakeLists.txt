cmake_minimum_required(VERSION 3.13)
project(TwurtleParserDependencies)

set(CMAKE_CXX_STANDARD 17)

file(GLOB dependencies */CMakeLists.txt)
foreach(dependency ${dependencies})
    get_filename_component(directory ${dependency} DIRECTORY)
    add_subdirectory(${directory})
endforeach()
