file(GLOB src_files "*.c" "*.cpp")
file(GLOB txt_files "*.json")

idf_component_register(
	SRCS ${src_files}
	INCLUDE_DIRS "."
	REQUIRES json NVS
	EMBED_TXTFILES ${txt_files}
)