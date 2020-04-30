cmake_minimum_required(VERSION 3.12)
project(sserialize-tools)

MACRO(ADD_TOOLS_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_TOOLS_TARGET_SINGLE)

MACRO(ADD_TOOLS_TARGET _name _sourcefiles)
	add_executable("${PROJECT_NAME}_${_name}" "${_sourcefiles}")
	target_link_libraries("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_TOOLS_TARGET)


add_tools_target_single(inspect_ItemIndexStore)
add_tools_target_single(compressedFileCreator)
add_tools_target_single(mmappedmem)
