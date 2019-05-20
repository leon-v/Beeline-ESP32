set(COMPONENT_ADD_INCLUDEDIRS ".")

file(GLOB component_files "*.c")
set(COMPONENT_SRCS ${component_files})

file(GLOB txt_files "./pages/*.html" "./pages/*.css" "./pages/*.js")
set(COMPONENT_EMBED_TXTFILES ${txt_files})

file(GLOB bin_files "./pages/*.png")
set(COMPONENT_EMBED_FILES ${bin_files})

set(COMPONENT_REQUIRES esp_http_server)
set(COMPONENT_PRIV_REQUIRES components nvs_flash)

register_component()
