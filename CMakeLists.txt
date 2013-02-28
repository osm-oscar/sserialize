cmake_minimum_required(VERSION 2.8)
project(sserialize)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(DEBUG_FLAGS "${DEBUG_FLAGS} -Wall")#-Wconversion -Wextra
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")

IF(NOT CMAKE_CROSSCOMPILING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nopie")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}
	${DEBUG_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	find_package(ICU 4.4 REQUIRED)
	set(OPTIONAL_SOURCES_CPP utility/DiacriticRemover.cpp)
ENDIF(NOT CMAKE_CROSSCOMPILING)

set(STATIC_TRIENODE_PRIVATE_SOURCES_CPP
Static/TrieNodePrivates/CompactTrieNodePrivate.cpp
Static/TrieNodePrivates/SimpleTrieNodePrivate.cpp)

set(STATIC_SOURCES_CPP
Static/Deque.cpp
Static/Set.cpp
Static/StringTable.cpp
Static/ItemIndexStore.cpp
Static/GeneralizedTrie.cpp
Static/FlatGeneralizedTrie.cpp
Static/GeoPoint.cpp
Static/triestats.cpp
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
)

set(SPATIAL_SOURCES_CPP
spatial/GeoShape.cpp
spatial/GeoPolygon.cpp
)

set(COMPLETER_SOURCES_CPP
completers/StringCompleter.cpp
completers/StringCompleterPrivate.cpp
completers/GeoCompleter.cpp
completers/GeoStringCompleter.cpp
)

set(UTILITY_SOURCES_CPP
utility/mmappedfile.cpp
utility/ChunkedMmappedFile.cpp
utility/CompressedMmappedFile.cpp
utility/pack_unpack_functions.cpp
utility/stringfunctions.cpp
utility/unicode_case_functions.cpp
utility/CompactUintArray.cpp
utility/UByteArrayAdapter.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateArray.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateMmappedFile.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateChunkedMmappedFile.cpp
utility/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateCompressedMmappedFile.cpp
utility/LinearRegressionFunctions.cpp
utility/histogram2d.cpp
utility/log.cpp
utility/utilfuncs.cpp)

set(EXTERNAL_SOURCES_CPP
vendor/libs/minilzo/minilzo.c
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
${EXTERNAL_SOURCES_CPP})

add_library(${PROJECT_NAME} STATIC ${SOURCES_CPP})

IF(NOT CMAKE_CROSSCOMPILING)
	if(ICU_FOUND)
		include_directories(${ICU_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} ${ICU_LIBRARIES})
	endif(ICU_FOUND)
	add_subdirectory(tests ${PROJECT_NAME}tests)
	add_subdirectory(tools ${PROJECT_NAME}-tools)
ENDIF(NOT CMAKE_CROSSCOMPILING)

add_subdirectory(benchmarks ${PROJECT_NAME}benchmarks)

