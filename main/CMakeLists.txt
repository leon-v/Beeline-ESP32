set(COMPONENT_ADD_INCLUDEDIRS .)

file(GLOB component_files "*.c")

set(COMPONENT_SRCS ${component_files})

register_component()