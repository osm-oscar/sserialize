cmake_minimum_required(VERSION 2.8)
project(sserializetests)
find_package(CPPUNIT)
find_package(ICU 4.9 REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${ICU_INCLUDE_DIRS})

set(TEST_LIB_SOURCES_CPP
trietestfuncs.cpp
printfunctions.cpp
utilalgos.cpp
TestItemData.cpp
datacreationfuncs.cpp
test_stringcompleter.cpp
)

add_library(${PROJECT_NAME} STATIC ${TEST_LIB_SOURCES_CPP})
add_dependencies(${PROJECT_NAME} sserialize)

MACRO(ADD_TEST_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${TEST_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserializetests)
ENDMACRO(ADD_TEST_TARGET_SINGLE)

MACRO(ADD_TEST_TARGET _name _sourcefiles)
	add_executable("${PROJECT_NAME}_${_name}" "${_sourcefiles}")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${TEST_LIB_LINK_LIBS})
	add_dependencies("${PROJECT_NAME}_${_name}" sserializetests)
ENDMACRO(ADD_TEST_TARGET)

# IF (CPPUNIT_FOUND)

set(TEST_LIB_LINK_LIBS
${sserializetests_BINARY_DIR}/libsserializetests.a
${sserialize_BINARY_DIR}/libsserialize.a
${CPPUNIT_LIBRARY}
${ICU_LIBRARIES}
${ICU_I18N_LIBRARIES}
)


#containers
ADD_TEST_TARGET_SINGLE(containers_ItemIndexFactory_Store)
ADD_TEST_TARGET_SINGLE(containers_itemindex)
ADD_TEST_TARGET_SINGLE(containers_gst)
ADD_TEST_TARGET_SINGLE(containers_ItemIndexIterator)
ADD_TEST_TARGET_SINGLE(containers_geostringsitemdb)
ADD_TEST_TARGET_SINGLE(containers_multivarbitarray)
ADD_TEST_TARGET_SINGLE(containers_ItemIndexWAH)
ADD_TEST_TARGET_SINGLE(containers_ItemIndexDE)
ADD_TEST_TARGET_SINGLE(containers_ItemIndexRleDE)
ADD_TEST_TARGET_SINGLE(containers_ItemIndexSimple)
ADD_TEST_TARGET_SINGLE(containers_DynamicBitSet)
ADD_TEST_TARGET_SINGLE(containers_SortedOffsetIndex)
ADD_TEST_TARGET_SINGLE(containers_setoptreesimple)
ADD_TEST_TARGET_SINGLE(containers_multibititerators)
ADD_TEST_TARGET_SINGLE(KeyValueObjectStoreTest)
ADD_TEST_TARGET_SINGLE(templated_WindowedArray)


#util
ADD_TEST_TARGET_SINGLE(util_compactuintarray)
ADD_TEST_TARGET_SINGLE(util_pack_unpack_functions)
ADD_TEST_TARGET_SINGLE(util_ubytearrayadapter)
ADD_TEST_TARGET_SINGLE(util_ChunkedMmappedFile)
ADD_TEST_TARGET_SINGLE(util_CompressedMmappedFile)
ADD_TEST_TARGET_SINGLE(util_utilfuncs)

#static
ADD_TEST_TARGET_SINGLE(static_deque)
ADD_TEST_TARGET_SINGLE(static_map)
ADD_TEST_TARGET_SINGLE(static_set)
ADD_TEST_TARGET_SINGLE(static_stringtable)
ADD_TEST_TARGET_SINGLE(static_trienodes)
ADD_TEST_TARGET_SINGLE(static_itemdb)
ADD_TEST_TARGET_SINGLE(static_stringsitemdb)
ADD_TEST_TARGET_SINGLE(static_geostringsitemdb)
ADD_TEST_TARGET_SINGLE(static_ItemGeoGrid)
ADD_TEST_TARGET_SINGLE(static_GeoStringsItemDB_GeoCompleter)
ADD_TEST_TARGET_SINGLE(static_GridRTree)
ADD_TEST_TARGET_SINGLE(templated_huffmancode)

#completers
ADD_TEST_TARGET_SINGLE(sc_gsa)
ADD_TEST_TARGET_SINGLE(sc_stringsitemdb)
ADD_TEST_TARGET_SINGLE(sc_ststringsitemdb)
ADD_TEST_TARGET_SINGLE(sc_gst)
ADD_TEST_TARGET_SINGLE(sc_gst_withdb)
ADD_TEST_TARGET_SINGLE(sc_gst_with_stitemdb)
ADD_TEST_TARGET_SINGLE(sc_stgst)
ADD_TEST_TARGET_SINGLE(sc_stgst_multiopts)
ADD_TEST_TARGET_SINGLE(sc_stflatgst)
ADD_TEST_TARGET_SINGLE(sc_stflatgst_strids)
ADD_TEST_TARGET_SINGLE(sc_SinglePassTrie)
# ADD_TEST_TARGET(sc_stgst_allopts)

#spatial
add_test_target_single(spatial_geopolygon)
add_test_target_single(spatial_polygonstore)

#misc
ADD_TEST_TARGET_SINGLE(unicodetest)
ADD_TEST_TARGET_SINGLE(util_memusage)
add_test_target_single(containers_itemindex_file)
add_test_target_single(containers_setoptree)

# ENDIF (CPPUNIT_FOUND)