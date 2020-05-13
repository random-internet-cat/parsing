set(ProjectName SimpleParser)
project(${ProjectName})

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE headers *.hpp)

add_executable(${ProjectName} ${sources} ${headers})

target_link_libraries(${ProjectName} RandomCat::Parser)
target_compile_options(
        ${ProjectName}
        PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        #        -Wconversion
        #                -Wsign-conversion
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnull-dereference
        -Wuseless-cast
        #                -Wdouble-promotion
        -Wformat=2
#        -Wlifetime
        -Weffc++
        -Wnarrowing
        -pedantic
        -pedantic-errors
)

target_include_directories(${ProjectName} PUBLIC include)

set(UseOptimizer TRUE)
set(UseLTO TRUE)
set(UseSanitizers TRUE)

if (UseOptimizer)
    target_compile_options(${ProjectName} PRIVATE -O3)

    if (UseLTO)
        target_compile_options(${ProjectName} PRIVATE -flto)
        target_link_options(${ProjectName} PRIVATE -flto)
    endif ()
else ()
    target_compile_options(${ProjectName} PRIVATE -O0)
endif ()

if (UseSanitizers)
#    set(SanitizerChecks address,undefined,integer)
    set(SanitizerChecks address,undefined)
    set(SanitizerArgs -fsanitize=${SanitizerChecks} #[[-fno-sanitize-recover=${SanitizerChecks}]])

    target_compile_options(${ProjectName} PRIVATE ${SanitizerArgs})
    target_link_options(${ProjectName} PRIVATE ${SanitizerArgs})
endif ()
