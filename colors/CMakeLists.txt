aest_add_library(colors
    MODULES src/lib.ixx
        src/Color.cppm
        src/Compositing.cppm
        src/Converter.cppm
        src/Cylindrical.cppm
        src/Distance.cppm
        src/NamedColors.cppm
        src/Format.cppm
        src/Gamut.cppm
        src/Operations.cppm
        src/Space.cppm
        src/Tonemap.cppm
        src/Transfer.cppm
    DEPENDS aest::blas
)

add_subdirectory(tests)

aest_add_examples(colors SOURCES examples/basic.cpp)
