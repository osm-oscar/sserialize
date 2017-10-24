cmake_minimum_required(VERSION 2.8)
project(sserialize-tools)
find_package(CPPUNIT)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${ICU_INCLUDE_DIRS})

MACRO(ADD_TOOLS_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${TOOLS_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_TOOLS_TARGET_SINGLE)

MACRO(ADD_TOOLS_TARGET _name _sourcefiles)
	add_executable("${PROJECT_NAME}_${_name}" "${_sourcefiles}")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${TOOLS_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_TOOLS_TARGET)

set(TOOLS_LIB_LINK_LIBS
	${SSERIALIZE_LINK_LIBRARIES}
	${LIBRT_LIBRARIES}
)


add_tools_target_single(inspect_ItemIndexStore)
add_tools_target_single(compressedFileCreator)
add_tools_target_single(mmappedmem)