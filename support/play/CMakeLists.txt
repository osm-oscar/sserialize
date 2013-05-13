cmake_minimum_required(VERSION 2.8)
project(sserialize-play)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

MACRO(ADD_PLAY_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${TOOLS_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_PLAY_TARGET_SINGLE)

MACRO(ADD_PLAY_TARGET _name _sourcefiles)
	add_executable("${PROJECT_NAME}_${_name}" "${_sourcefiles}")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${TOOLS_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_PLAY_TARGET)

set(PLAY_LIB_LINK_LIBS
${sserialize_BINARY_DIR}/libsserialize.a)


add_tools_target_single(flatgst_prototype)