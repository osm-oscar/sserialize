cmake_minimum_required(VERSION 2.8)
project(sserialize)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -Wall")

IF(NOT CMAKE_CROSSCOMPILING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	find_package(ICU 4.4 REQUIRED)
	set(OPTIONAL_SOURCES_CPP
	containers/GeneralizedTrie/SinglePassTrie.cpp
	utility/DiacriticRemover.cpp
	)
ENDIF(NOT CMAKE_CROSSCOMPILING)

set(RAGEL_FLAGS "-G2")

RAGEL_PARSER(containers/SetOpTreePrivateSimple_parser.rl)

set(STATIC_TRIENODE_PRIVATE_SOURCES_CPP
Static/TrieNodePrivates/CompactTrieNodePrivate.cpp
Static/TrieNodePrivates/SimpleTrieNodePrivate.cpp
Static/TrieNodePrivates/LargeCompactTrieNodePrivate.cpp
)

set(STAGING_SOURCES_CPP
staging/containers/DataSetFactory.cpp
)

set(STATIC_SOURCES_CPP
Static/Set.cpp
Static/StringTable.cpp
Static/ItemIndexStore.cpp
Static/StringCompleter.cpp
Static/GeneralizedTrie.cpp
Static/FlatGeneralizedTrie.cpp
Static/triestats.cpp
Static/HuffmanDecoder.cpp
Static/KeyValueObjectStore.cpp
Static/DynamicKeyValueObjectStore.cpp
Static/GeoShape.cpp
Static/RTree.cpp
Static/HierarchyCompleter.cpp
Static/GeoHierarchy.cpp
Static/GeoWay.cpp
Static/DenseGeoPointVector.cpp
Static/GeoPolygon.cpp
Static/GeoMultiPolygon.cpp
Static/UnicodeTrie/detail/SimpleNode.cpp
Static/UnicodeTrie/detail/Node.cpp
Static/UnicodeTrie/detail/EmptyNode.cpp
Static/UnicodeTrie/FlatTrie.cpp
Static/GeoHierarchySubSetCreator.cpp
Static/Triangulation.cpp
Static/TriangulationGridLocator.cpp
Static/TriangulationRegionArrangement.cpp
Static/TriangulationGeoHierarchyArrangement.cpp
)

set(ITEM_INDEX_SOURCES_CPP
containers/ItemIndex.cpp
containers/ItemIndexPrivates/ItemIndexPrivate.cpp
containers/ItemIndexPrivates/ItemIndexPrivateRegLine.cpp
containers/ItemIndexPrivates/ItemIndexPrivateSimple.cpp
containers/ItemIndexPrivates/ItemIndexPrivateStlDeque.cpp
containers/ItemIndexPrivates/ItemIndexPrivateStlVector.cpp
containers/ItemIndexPrivates/ItemIndexSetFunctions.cpp
containers/ItemIndexPrivates/ItemIndexPrivateWAH.cpp
containers/ItemIndexPrivates/ItemIndexPrivateDE.cpp
containers/ItemIndexPrivates/ItemIndexPrivateRleDE.cpp
containers/ItemIndexPrivates/ItemIndexPrivateNative.cpp
)

set(CONTAINTERS_SOURCES_CPP 
containers/ItemIndexFactory.cpp
containers/SetOpTree.cpp
containers/SetOpTreePrivateComplex.cpp
containers/SetOpTreePrivateSimple.cpp
containers/SetOpTreePrivate_parser.cpp
containers/ItemIndexIterator.cpp
containers/ItemIndexIteratorSetOp.cpp
containers/ItemIndexIteratorIntersecting.cpp
containers/MultiVarBitArray.cpp
containers/DynamicBitSet.cpp
containers/SortedOffsetIndex.cpp
containers/SortedOffsetIndexPrivate.cpp
containers/SetOpTreePrivateSimple_parser.cpp
containers/MultiBitBackInserter.cpp
containers/MultiBitIterator.cpp
containers/UDWIterator.cpp
containers/UDWIteratorPrivateHD.cpp
containers/KeyValueObjectStore.cpp
containers/CellQueryResult.cpp
containers/CellQueryResultPrivate.cpp
containers/OpTree.cpp
containers/TreedCQRImp.cpp
containers/TreedCQR.cpp
)

set(SPATIAL_SOURCES_CPP
spatial/GeoPoint.cpp
spatial/GeoWay.cpp
spatial/GeoRect.cpp
spatial/GeoPolygon.cpp
spatial/GeoMultiPolygon.cpp
spatial/RTree.cpp
spatial/GridRTree.cpp
spatial/GeoHierarchy.cpp
spatial/GridRegionTree.cpp
spatial/GeoGrid.cpp
spatial/DistanceCalculator.cpp
spatial/LatLonCalculations.cpp
)

set(COMPLETER_SOURCES_CPP
completers/StringCompleter.cpp
completers/StringCompleterPrivate.cpp
completers/GeoCompleter.cpp
completers/GeoStringCompleter.cpp
completers/StringCompleterPrivateMulti.cpp
completers/StringCompleterPrivateGeoHierarchyUnclustered.cpp
)

set(UTILITY_SOURCES_CPP
utility/mmappedfile.cpp
utility/ChunkedMmappedFile.cpp
utility/CompressedMmappedFile.cpp
utility/stringfunctions.cpp
utility/unicode_case_functions.cpp
utility/CompactUintArray.cpp
utility/UByteArrayAdapter.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivate.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterSeekedFile.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateArray.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateMmappedFile.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateChunkedMmappedFile.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateCompressedMmappedFile.cpp
utility/histogram2d.cpp
utility/log.cpp
utility/FileHandler.cpp
utility/Compressor.cpp
utility/detail/Compressor/NoneCompressor.cpp
utility/detail/Compressor/LZOCompressor.cpp
utility/MultiReaderSingleWriterLock.cpp
utility/ProgreessInfo.cpp
utility/TimeMeasurer.cpp
utility/RLEStream.cpp
utility/ThreadPool.cpp
)

set(EXTERNAL_SOURCES_CPP
vendor/libs/minilzo/minilzo.c
vendor/libs/geographiclib/legacy/C/geodesic.c
)

set(SOURCES_CPP
${COMPLETER_SOURCES_CPP}
${STATIC_TRIENODE_PRIVATE_SOURCES_CPP}
${STATIC_SOURCES_CPP}
${CONTAINTERS_SOURCES_CPP}
${SPATIAL_SOURCES_CPP}
${ITEM_INDEX_SOURCES_CPP}
${UTILITY_SOURCES_CPP}
${OPTIONAL_SOURCES_CPP}
${EXTERNAL_SOURCES_CPP}
${STAGING_SOURCES_CPP})

add_library(${PROJECT_NAME} STATIC ${SOURCES_CPP})

IF(NOT CMAKE_CROSSCOMPILING)
	if(ICU_FOUND)
		include_directories(${ICU_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} ${ICU_LIBRARIES} ${LIBRT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	endif(ICU_FOUND)
	add_subdirectory(tests ${PROJECT_NAME}tests EXCLUDE_FROM_ALL)
	add_subdirectory(tools ${PROJECT_NAME}-tools EXCLUDE_FROM_ALL)
	add_subdirectory(benchmarks ${PROJECT_NAME}benchmarks EXCLUDE_FROM_ALL)
ENDIF(NOT CMAKE_CROSSCOMPILING)
