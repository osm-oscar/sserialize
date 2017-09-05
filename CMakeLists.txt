cmake_minimum_required(VERSION 3.0)
project(sserialize)
set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

MACRO(ADD_TARGET_PROPERTIES _target _name)
	SET(_properties)
	FOREACH(_prop ${ARGN})
		SET(_properties "${_properties} ${_prop}")
	ENDFOREACH(_prop)
	GET_TARGET_PROPERTY(_old_properties ${_target} ${_name})
	MESSAGE(STATUS "adding property to ${_target} ${_name}:" ${_properties})
	IF(NOT _old_properties)
		# in case it's NOTFOUND
		SET(_old_properties)
	ENDIF(NOT _old_properties)
	SET_TARGET_PROPERTIES(${_target} PROPERTIES ${_name} "${_old_properties} ${_properties}")
ENDMACRO(ADD_TARGET_PROPERTIES)

set(MY_COMPILE_DEFINITIONS)

IF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_CONTIGUOUS_UBA_ONLY" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_ENABLED)

IF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED)

IF (SSERIALIZE_UBA_OPTIONAL_REFCOUNTING_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_UBA_OPTIONAL_REFCOUNTING" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_UBA_OPTIONAL_REFCOUNTING_ENABLED)

IF (SSERIALIZE_CHEAP_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_CHEAP_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_CHEAP_ASSERT_ENABLED)

IF (SSERIALIZE_NORMAL_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_NORMAL_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_NORMAL_ASSERT_ENABLED)

IF (SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_EXPENSIVE_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)

add_subdirectory(vendor/libdts2 ${PROJECT_NAME}dts2)

set(SSERIALIZE_INCLUDE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	${LIBDTS2_INCLUDE_DIR}
	CACHE STRING "sserialize include directories"
	FORCE
)

set(SSERIALIZE_LIBRARIES
	${PROJECT_NAME}
	CACHE STRING "sserialize libraries"
	FORCE
)

set(SSERIALIZE_LINK_LIBRARIES
	${PROJECT_NAME}
	${LIBDTS2_LINK_LIBRARIES}
	CACHE STRING "sserialize link libraries"
	FORCE
)

set(SSERIALIZE_COMPILE_DEFINITIONS
	${LIBDTS2_COMPILE_DEFINITIONS}
	${MY_COMPILE_DEFINITIONS}
	CACHE STRING "sserialize compile definitions"
	FORCE
)

mark_as_advanced(
	SSERIALIZE_INCLUDE_DIR
	SSERIALIZE_LIBRARIES
	SSERIALIZE_LINK_LIBRARIES
	SSERIALIZE_COMPILE_DEFINITIONS
)

include_directories(
	${SSERIALIZE_INCLUDE_DIR}
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor/libs"
)

IF(CMAKE_CROSSCOMPILING)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM_BEFORE ${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM})
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH) 
	find_package(Ragel REQUIRED)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM_BEFORE})
ELSE(CMAKE_CROSSCOMPILING)
	find_package(LIBRT REQUIRED)
	find_package(Ragel REQUIRED)
	find_package(Threads)
ENDIF(CMAKE_CROSSCOMPILING)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall")

IF(NOT CMAKE_CROSSCOMPILING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	find_package(ICU 4.4 REQUIRED)
	set(OPTIONAL_SOURCES_CPP
		src/containers/GeneralizedTrie/SinglePassTrie.cpp
		src/strings/DiacriticRemover.cpp
	)
ENDIF(NOT CMAKE_CROSSCOMPILING)

set(RAGEL_FLAGS "-G2")

RAGEL_PARSER(src/search/SetOpTreePrivateSimple_parser.rl)

set(STORAGE_SOURCES_CPP
	src/storage/MmappedFile.cpp
	src/storage/ChunkedMmappedFile.cpp
	src/storage/CompressedMmappedFile.cpp
	src/storage/UByteArrayAdapter.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivate.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterThreadSafeFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateArray.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateMmappedFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateChunkedMmappedFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateCompressedMmappedFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateVector.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterMM.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterMV.cpp
	src/storage/FileHandler.cpp
)

set(MT_SOURCES_CPP
	src/mt/MultiReaderSingleWriterLock.cpp
	src/mt/ThreadPool.cpp
)

set(STATS_SOURCES_CPP
	src/stats/histogram2d.cpp
	src/stats/ProgreessInfo.cpp
	src/stats/TimeMeasurer.cpp
)

set(STATIC_SOURCES_CPP
	src/Static/CellTextCompleter.cpp
	src/Static/TrieNodePrivates/CompactTrieNodePrivate.cpp
	src/Static/TrieNodePrivates/SimpleTrieNodePrivate.cpp
	src/Static/TrieNodePrivates/LargeCompactTrieNodePrivate.cpp
	src/Static/Set.cpp
	src/Static/StringTable.cpp
	src/Static/ItemIndexStore.cpp
	src/Static/StringCompleter.cpp
	src/Static/GeneralizedTrie.cpp
	src/Static/FlatGeneralizedTrie.cpp
	src/Static/triestats.cpp
	src/Static/HuffmanDecoder.cpp
	src/Static/KeyValueObjectStore.cpp
	src/Static/DynamicKeyValueObjectStore.cpp
	src/Static/GeoShape.cpp
	src/Static/RTree.cpp
	src/Static/HierarchyCompleter.cpp
	src/Static/GeoHierarchy.cpp
	src/Static/GeoWay.cpp
	src/Static/DenseGeoPointVector.cpp
	src/Static/GeoPolygon.cpp
	src/Static/GeoMultiPolygon.cpp
	src/Static/CQRDilator.cpp
	src/Static/UnicodeTrie/detail/SimpleNode.cpp
	src/Static/UnicodeTrie/detail/Node.cpp
	src/Static/UnicodeTrie/detail/EmptyNode.cpp
	src/Static/UnicodeTrie/FlatTrie.cpp
	src/Static/GeoHierarchySubGraph.cpp
	src/Static/Triangulation.cpp
	src/Static/TriangulationGridLocator.cpp
	src/Static/TriangulationRegionArrangement.cpp
	src/Static/TriangulationGeoHierarchyArrangement.cpp
	src/Static/TracGraph.cpp
	src/Static/Fraction.cpp
	src/Static/PointOnS2.cpp
)

set(CONTAINTERS_SOURCES_CPP 
	src/containers/CompactUintArray.cpp
	src/containers/ItemIndexFactory.cpp
	src/containers/ItemIndexIterator.cpp
	src/containers/ItemIndexIteratorSetOp.cpp
	src/containers/ItemIndexIteratorIntersecting.cpp
	src/containers/MultiVarBitArray.cpp
	src/containers/DynamicBitSet.cpp
	src/containers/SortedOffsetIndex.cpp
	src/containers/SortedOffsetIndexPrivate.cpp
	src/containers/KeyValueObjectStore.cpp
	src/containers/VariantStore.cpp
	src/containers/RLEStream.cpp
	src/containers/SimpleBitVector.cpp
	src/containers/ItemIndex.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivate.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateSimple.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateStlDeque.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateStlVector.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateWAH.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateDE.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateNative.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateEliasFano.cpp
)

set(SPATIAL_SOURCES_CPP
	src/spatial/GeoShape.cpp
	src/spatial/GeoNone.cpp
	src/spatial/GeoPoint.cpp
	src/spatial/GeoWay.cpp
	src/spatial/GeoRect.cpp
	src/spatial/GeoPolygon.cpp
	src/spatial/GeoMultiPolygon.cpp
	src/spatial/GeoUnionShape.cpp
	src/spatial/RTree.cpp
	src/spatial/GridRTree.cpp
	src/spatial/GeoHierarchy.cpp
	src/spatial/GridRegionTree.cpp
	src/spatial/GeoGrid.cpp
	src/spatial/DistanceCalculator.cpp
	src/spatial/LatLonCalculations.cpp
	src/spatial/CellQueryResult.cpp
	src/spatial/CellQueryResultPrivate.cpp
	src/spatial/TreedCQRImp.cpp
	src/spatial/TreedCQR.cpp
	src/spatial/PointOnS2.cpp
)

set(ITERATOR_SOURCES_CPP
	src/iterator/MultiBitBackInserter.cpp
	src/iterator/MultiBitIterator.cpp
	src/iterator/UDWIterator.cpp
	src/iterator/UDWIteratorPrivateHD.cpp
	src/iterator/UnaryCodeIterator.cpp
)

set(SEARCH_SOURCES_CPP
	src/search/StringCompleter.cpp
	src/search/StringCompleterPrivate.cpp
	src/search/GeoCompleter.cpp
	src/search/GeoStringCompleter.cpp
	src/search/StringCompleterPrivateMulti.cpp
	src/search/StringCompleterPrivateGeoHierarchyUnclustered.cpp
	src/search/OpTree.cpp
	src/search/SetOpTreePrivateSimple_parser.cpp
	src/search/SetOpTree.cpp
	src/search/SetOpTreePrivateComplex.cpp
	src/search/SetOpTreePrivateSimple.cpp
	src/search/SetOpTreePrivate_parser.cpp
)

set(STRINGS_SOURCES_CPP
	src/strings/stringfunctions.cpp
	src/strings/unicode_case_functions.cpp
)

set(UTILITY_SOURCES_CPP
	src/utility/log.cpp
	src/utility/Compressor.cpp
	src/utility/detail/Compressor/NoneCompressor.cpp
	src/utility/detail/Compressor/LZOCompressor.cpp
	src/utility/Fraction.cpp
	src/utility/exceptions.cpp
)

set(STAGING_SOURCES_CPP
)

set(EXTERNAL_SOURCES_CPP
	vendor/libs/minilzo/minilzo.c
	vendor/libs/geographiclib/legacy/C/geodesic.c
)

file(GLOB_RECURSE INCLUDE_HEADERS include/*.h)

set(SOURCES_CPP
	${STORAGE_SOURCES_CPP}
	${MT_SOURCES_CPP}
	${STATS_SOURCES_CPP}
	${STATIC_SOURCES_CPP}
	${CONTAINTERS_SOURCES_CPP}
	${SPATIAL_SOURCES_CPP}
	${ITERATOR_SOURCES_CPP}
	${SEARCH_SOURCES_CPP}
	${STRINGS_SOURCES_CPP}
	${UTILITY_SOURCES_CPP}
	${OPTIONAL_SOURCES_CPP}
	${EXTERNAL_SOURCES_CPP}
	${STAGING_SOURCES_CPP}
	${INCLUDE_HEADERS}
)

add_library(${PROJECT_NAME} STATIC ${SOURCES_CPP})
target_link_libraries(${PROJECT_NAME} dts2)
target_compile_definitions(${PROJECT_NAME} PUBLIC ${SSERIALIZE_COMPILE_DEFINITIONS})
add_target_properties(${PROJECT_NAME} COMPILE_FLAGS -fPIC)

IF(NOT CMAKE_CROSSCOMPILING)
	if(ICU_FOUND)
		include_directories(${ICU_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} ${ICU_LIBRARIES} ${LIBRT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	endif(ICU_FOUND)
	add_subdirectory(tests ${PROJECT_NAME}tests EXCLUDE_FROM_ALL)
	add_subdirectory(tools ${PROJECT_NAME}-tools EXCLUDE_FROM_ALL)
	add_subdirectory(benchmarks ${PROJECT_NAME}benchmarks EXCLUDE_FROM_ALL)
ENDIF(NOT CMAKE_CROSSCOMPILING)
