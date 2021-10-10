file(GLOB_RECURSE srcs *.c *.cpp)

file(GLOB_RECURSE
    include_dirs
    LIST_DIRECTORIES true
    .
)


idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${include_dirs})
