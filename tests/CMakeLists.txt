cmake_minimum_required(VERSION 3.9)
project(sserializetests)
find_package(CPPUNIT)

IF (CPPUNIT_FOUND)

set(MY_INCLUDE_DIRS ${CPPUNIT_INCLUDE_DIRS})

set(TEST_LIB_SOURCES_CPP
	trietestfuncs.cpp
	printfunctions.cpp
	utilalgos.cpp
	TestItemData.cpp
	datacreationfuncs.cpp
	test_stringcompleter.cpp
	TestBase.cpp
	helpers.cpp
)

set(MY_LINK_LIBS
	sserialize
	${CPPUNIT_LIBRARY}
)

add_library(${PROJECT_NAME} STATIC ${TEST_LIB_SOURCES_CPP})
target_link_libraries(${PROJECT_NAME} PUBLIC ${MY_LINK_LIBS})
target_include_directories(${PROJECT_NAME} PUBLIC ${MY_INCLUDE_DIRS})

MACRO(ADD_TEST_TARGET_SINGLE _name)
	add_executable("${PROJECT_NAME}_${_name}" "${_name}.cpp")
	target_link_libraries("${PROJECT_NAME}_${_name}" ${PROJECT_NAME})
ENDMACRO(ADD_TEST_TARGET_SINGLE)

#containers
ADD_TEST_TARGET_SINGLE(containers_ItemIndexFactory_Store)
ADD_TEST_TARGET_SINGLE(containers_ItemIndex)
ADD_TEST_TARGET_SINGLE(containers_ItemIndexIterator)
ADD_TEST_TARGET_SINGLE(containers_geostringsitemdb)
ADD_TEST_TARGET_SINGLE(containers_multivarbitarray)
ADD_TEST_TARGET_SINGLE(containers_DynamicBitSet)
ADD_TEST_TARGET_SINGLE(containers_SortedOffsetIndex)
ADD_TEST_TARGET_SINGLE(containers_setoptreesimple)
ADD_TEST_TARGET_SINGLE(containers_multibititerators)
ADD_TEST_TARGET_SINGLE(KeyValueObjectStoreTest)
ADD_TEST_TARGET_SINGLE(templated_WindowedArray)
ADD_TEST_TARGET_SINGLE(containers_HashBasedFlatTrie)
ADD_TEST_TARGET_SINGLE(containers_VariantStore)
ADD_TEST_TARGET_SINGLE(containers_CFLArray)
ADD_TEST_TARGET_SINGLE(containers_OOMArray)
ADD_TEST_TARGET_SINGLE(containers_OOMFlatTrie)

#util
ADD_TEST_TARGET_SINGLE(util_compactuintarray)
ADD_TEST_TARGET_SINGLE(util_pack_unpack_functions)
ADD_TEST_TARGET_SINGLE(util_ubytearrayadapter)
ADD_TEST_TARGET_SINGLE(util_ChunkedMmappedFile)
ADD_TEST_TARGET_SINGLE(util_CompressedMmappedFile)
ADD_TEST_TARGET_SINGLE(util_utilfuncs)
ADD_TEST_TARGET_SINGLE(util_packfuncs)
ADD_TEST_TARGET_SINGLE(util_LinearRegregionnFunctions)
ADD_TEST_TARGET_SINGLE(util_ThreadPool)
ADD_TEST_TARGET_SINGLE(AsciiCharEscaper)
ADD_TEST_TARGET_SINGLE(util_MmappedMemory)
ADD_TEST_TARGET_SINGLE(util_RLEStream)
ADD_TEST_TARGET_SINGLE(algorithm_oom_sort)
ADD_TEST_TARGET_SINGLE(util_UByteArrayAdapter)

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
ADD_TEST_TARGET_SINGLE(static_DynamicVector)
ADD_TEST_TARGET_SINGLE(DynamicKeyValueObjectStoreTest)
ADD_TEST_TARGET_SINGLE(static_densegeopointvector)
ADD_TEST_TARGET_SINGLE(static_GeoShapes)

#completers
ADD_TEST_TARGET_SINGLE(sc_stringsitemdb)
ADD_TEST_TARGET_SINGLE(sc_ststringsitemdb)
ADD_TEST_TARGET_SINGLE(sc_SinglePassTrie)
ADD_TEST_TARGET_SINGLE(UnicodeTrieTest)
ADD_TEST_TARGET_SINGLE(StaticUnicodeTrieTest)

#spatial
add_test_target_single(spatial_geopolygon)
add_test_target_single(spatial_polygonstore)
add_test_target_single(spatial_GridRegionTree)
add_test_target_single(spatial_GeoPolygon)

#misc
ADD_TEST_TARGET_SINGLE(unicodetest)
ADD_TEST_TARGET_SINGLE(util_memusage)
add_test_target_single(containers_itemindex_file)
add_test_target_single(containers_setoptree)

#iterators
ADD_TEST_TARGET_SINGLE(iterator_UnaryCodeIterator)

#staging
ADD_TEST_TARGET_SINGLE(staging_GenericTrie)

#stuff with cgal deps
IF(CGAL_FOUND)

ADD_TEST_TARGET_SINGLE(sort_oom_sactc_data)
ADD_TEST_TARGET_SINGLE(search_CellTextCompleter)
ADD_TEST_TARGET_SINGLE(static_Triangulation)

ENDIF(CGAL_FOUND)

ELSE()
	message(WARNING "Could not find cppunit library needed to compile the tests")
ENDIF (CPPUNIT_FOUND)
