aest_add_library(blas
    MODULES src/lib.ixx
        src/Matrix.cppm
)

add_subdirectory(tests)

aest_add_examples(blas SOURCES examples/basic.cpp)
