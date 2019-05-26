project(RandomCatParser)

file(GLOB_RECURSE headers include/*.hpp)
file(GLOB_RECURSE impl_headers include*.inl)
file(GLOB_RECURSE sources src/*.cpp)

add_library(__RC_Parser ${sources} ${headers} ${impl_headers})

target_include_directories(__RC_Parser PUBLIC include)
set_target_properties(__RC_Parser PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(__RC_Parser GSL RandomCat::AllLibraries)
target_compile_options(__RC_Parser PRIVATE -Wall -Wextra)

add_library(RandomCat::Parser ALIAS __RC_Parser)
