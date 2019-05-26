set(COMPONENT_ADD_INCLUDEDIRS "." "./driver/sx1278")

file(GLOB component_files "*.c" "./driver/*/*.c")
set(COMPONENT_SRCS ${component_files})

file(GLOB txt_files "*.html" "*.css" "*.js")
set(COMPONENT_EMBED_TXTFILES ${txt_files})

file(GLOB bin_files "*.png")
set(COMPONENT_EMBED_FILES ${bin_files})

set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES components device wpa_supplicant)

register_component()
