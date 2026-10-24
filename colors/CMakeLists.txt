aest_add_library(colors
    MODULES src/lib.ixx
        src/Color.cppm
        src/Converter.cppm
        src/Distance.cppm
        src/NamedColors.cppm
        src/Format.cppm
        src/Operations.cppm
        src/Space.cppm
)

add_subdirectory(tests)

aest_add_examples(colors SOURCES examples/basic.cpp)
