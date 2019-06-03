set(COMPONENT_ADD_INCLUDEDIRS ".")

file(GLOB component_files "*.c")
set(COMPONENT_SRCS ${component_files})

file(GLOB txt_files "*.html" "*.css" "*.js")
set(COMPONENT_EMBED_TXTFILES ${txt_files})

file(GLOB bin_files "*.png")
set(COMPONENT_EMBED_FILES ${bin_files})

set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES components device)

register_component()
