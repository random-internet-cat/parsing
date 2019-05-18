project(TwurtlePraser)

file(GLOB_RECURSE headers include/*.hpp)
file(GLOB_RECURSE impl_headers include*.inl)
file(GLOB_RECURSE sources src/*.cpp)

add_library(__MT_Parser ${sources} ${headers} ${impl_headers})

target_include_directories(__MT_Parser PUBLIC include)
set_target_properties(__MT_Parser PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(__MT_Parser GSL)
target_compile_options(__MT_Parser PRIVATE -Wall -Wextra)

add_library(MeowingTwurtle::Parser ALIAS __MT_Parser)
