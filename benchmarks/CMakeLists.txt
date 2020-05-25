cmake_minimum_required(VERSION 3.12)
project(sserialize-bench)

find_package(LIBRT REQUIRED)
find_package(Threads)

IF(ANDROID)
	SET(BENCH_LIB_LINK_LIBS
		sserialize
	)
ELSE(ANDROID)
	set(BENCH_LIB_LINK_LIBS
		sserialize
		${LIBRT_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT})
ENDIF(ANDROID)

MACRO(ADD_BENCH_TARGET _name _sourcefiles)
	add_executable("${PROJECT_NAME}_${_name}" "${_sourcefiles}")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${BENCH_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
	set(SSERIALIZEBENCH_ALL_TARGETS ${SSERIALIZEBENCH_ALL_TARGETS} "${PROJECT_NAME}_${_name}")
ENDMACRO(ADD_BENCH_TARGET)

MACRO(ADD_BENCH_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${BENCH_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserialize)
	set(SSERIALIZEBENCH_ALL_TARGETS ${SSERIALIZEBENCH_ALL_TARGETS} "${PROJECT_NAME}_${_name}")
ENDMACRO(ADD_BENCH_TARGET_SINGLE)

ADD_BENCH_TARGET_SINGLE(itemindex)
ADD_BENCH_TARGET_SINGLE(dynamic_bitset_benchmark)
ADD_BENCH_TARGET_SINGLE(dynamic_bitset_file_bench)
ADD_BENCH_TARGET_SINGLE(coding)
ADD_BENCH_TARGET_SINGLE(oom_sort)
ADD_BENCH_TARGET_SINGLE(itemindex-for)

add_custom_target(${PROJECT_NAME}_all DEPENDS ${SSERIALIZEBENCH_ALL_TARGETS})
