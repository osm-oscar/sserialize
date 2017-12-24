cmake_minimum_required(VERSION 2.8)
project(benchmarks)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

include_directories(
	${PROJECT_GLOBAL_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
)

find_package(LIBRT REQUIRED)
find_package(Threads)

IF(ANDROID)
	SET(BENCH_LIB_LINK_LIBS
		${SSERIALIZE_LINK_LIBRARIES}
	)
ELSE(ANDROID)
	set(BENCH_LIB_LINK_LIBS
		${SSERIALIZE_LINK_LIBRARIES}
		${LIBRT_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT})
ENDIF(ANDROID)

MACRO(ADD_BENCH_TARGET _name _sourcefiles)
	add_executable("${PROJECT_NAME}_${_name}" "${_sourcefiles}")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${BENCH_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_BENCH_TARGET)

MACRO(ADD_BENCH_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${BENCH_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
ENDMACRO(ADD_BENCH_TARGET_SINGLE)

ADD_BENCH_TARGET_SINGLE(itemindex)
ADD_BENCH_TARGET_SINGLE(dynamic_bitset_benchmark)
ADD_BENCH_TARGET_SINGLE(dynamic_bitset_file_bench)
ADD_BENCH_TARGET_SINGLE(uint_dee_encode_bench)
ADD_BENCH_TARGET_SINGLE(oom_sort)