aest_add_library(colors
    MODULES src/lib.ixx
        src/CieLab.cppm
        src/Color.cppm
        src/Compositing.cppm
        src/Converter.cppm
        src/Cylindrical.cppm
        src/Distance.cppm
        src/NamedColors.cppm
        src/OkLab.cppm
        src/Format.cppm
        src/Gamut.cppm
        src/Hex.cppm
        src/Operations.cppm
        src/Packed.cppm
        src/Random.cppm
        src/Space.cppm
        src/Tonemap.cppm
        src/Transfer.cppm
    DEPENDS aest::blas
)

add_subdirectory(tests)

aest_add_examples(colors
    SOURCES examples/basic.cpp
        examples/gradient.cpp
        examples/hdr.cpp
        examples/palette.cpp
        examples/theme.cpp
)
