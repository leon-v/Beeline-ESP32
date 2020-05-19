file(GLOB src_files "*.c" "*.cpp")
file(GLOB txt_files "*.json")

idf_component_register(
	SRCS ${src_files}
	INCLUDE_DIRS "."
	REQUIRES Module nvs_flash json
	EMBED_TXTFILES ${txt_files}
)