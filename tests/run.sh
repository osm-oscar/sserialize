#!/bin/bash

DISABLED_TESTS=(
	"sserializetests_staging_GenericTrie"
)

SLOW_TESTS=("sserializetests_algorithm_oom_sort"
	"sserializetests_containers_ItemIndexFactory_Store"
	"sserializetests_containers_OOMArray"
	"sserializetests_containers_SortedOffsetIndex"
	"sserializetests_search_CellTextCompleter"
	"sserializetests_util_ChunkedMmappedFile"
	"sserializetests_util_compactuintarray"
	"sserializetests_util_CompressedMmappedFile"
	"sserializetests_util_MmappedMemory"
	"sserializetests_static_Triangulation"
)

NORMAL_TESTS=(
	"sserializetests_containers_ItemIndex"
	"sserializetests_containers_multivarbitarray"
	"sserializetests_containers_VariantStore"
	"sserializetests_static_deque"
	"sserializetests_templated_huffmancode"
)

FAST_TESTS=(
	"sserializetests_AsciiCharEscaper"
	"sserializetests_containers_CFLArray"
	"sserializetests_containers_DynamicBitSet"
	"sserializetests_containers_geostringsitemdb"
	"sserializetests_containers_HashBasedFlatTrie"
	"sserializetests_containers_ItemIndexIterator"
	"sserializetests_containers_multibititerators"
	"sserializetests_containers_setoptreesimple"
	"sserializetests_sc_SinglePassTrie"
	"sserializetests_sc_stringsitemdb"
	"sserializetests_sc_ststringsitemdb"
	"sserializetests_spatial_geopolygon"
	"sserializetests_spatial_GeoPolygon"
	"sserializetests_spatial_GridRegionTree"
	"sserializetests_spatial_polygonstore"
	"sserializetests_static_densegeopointvector"
	"sserializetests_static_DynamicVector"
	"sserializetests_static_GeoShapes"
	"sserializetests_static_geostringsitemdb"
	"sserializetests_static_GeoStringsItemDB_GeoCompleter"
	"sserializetests_static_GridRTree"
	"sserializetests_static_ItemGeoGrid"
	"sserializetests_static_map"
	"sserializetests_static_set"
	"sserializetests_static_stringsitemdb"
	"sserializetests_static_stringtable"
	"sserializetests_static_trienodes"
	"sserializetests_StaticUnicodeTrieTest"
	"sserializetests_templated_WindowedArray"
	"sserializetests_unicodetest"
	"sserializetests_UnicodeTrieTest"
	"sserializetests_util_LinearRegregionnFunctions"
	"sserializetests_util_memusage"
	"sserializetests_util_packfuncs"
	"sserializetests_util_pack_unpack_functions"
	"sserializetests_util_RLEStream"
	"sserializetests_util_ThreadPool"
	"sserializetests_util_UByteArrayAdapter"
	"sserializetests_util_utilfuncs"
)

PARAM_TESTS=(
	"sserializetests_containers_itemindex_file"
	"sserializetests_DynamicKeyValueObjectStoreTest"
	"sserializetests_KeyValueObjectStoreTest"
	"sserializetests_sort_oom_sactc_data"
	"sserializetests_static_itemdb"
	"sserializetests_util_ubytearrayadapter"
)

SHM_FILE_PREFIX="sserializeteststmp"

BUILD_PATH="${1}"

TEST_SELECTION="${2}"

if [ -z "${TEST_SELECTION}" ]; then
	echo "No test selected. Select with run.sh build-path SELECTION where selection is a combination of snf";
	exit 1
fi

if [ ! -d "${BUILD_PATH}" ] || [ ! -f "${BUILD_PATH}/sserializetests_util_UByteArrayAdapter" ]; then
	echo "Invalid build path given"
	exit 1
fi

cd "${BUILD_PATH}" && echo "Entering build path: ${BUILD_PATH}"

if [ ! -d "${BUILD_PATH}/testtmp/slow" ]; then
	mkdir -p "${BUILD_PATH}/testtmp/slow" || exit 1
else
	rm "${BUILD_PATH}/testtmp/slow/*"
fi

if [ ! -d "${BUILD_PATH}/testtmp/fast" ]; then
	mkdir -p "${BUILD_PATH}/testtmp/fast" || exit 1
else
	rm "${BUILD_PATH}/testtmp/fast/*"
fi

rm /dev/shm/sserializeteststmp*

TMP_FILE_OPTS="--tc-fast-temp-file ${BUILD_PATH$}/testtmp/fast/f --tc-slow-temp-file ${BUILD_PATH$}/testtmp/slow/f --tc-shm-file sserializeteststmp"

num_slow=${#SLOW_TESTS[@]}
num_normal=${#NORMAL_TESTS[@]}
num_fast=${#FAST_TESTS[@]}
TEST_FAILED=0

echo "#Slow tests: $num_slow"
echo "#Normal tests: $num_normal"
echo "#Fast tests: $num_fast"

echo "${TEST_SELECTION}" | egrep "f"
if [ $? -eq 0 ]; then
	for ((i = 0; i < ${#FAST_TESTS[@]}; ++i)); do
		echo "Executing $i: ${FAST_TESTS[i]}"
		eval "./${FAST_TESTS[i]} ${TMP_FILE_OPTS}"
		FAST_TESTS_RESULT[i]=$?
		ls -1 /tmp/sserialize*
		if [ $? -eq 0 ]; then
			echo "tmp broken for $i"
			exit 1
		fi
	done
fi

echo "${TEST_SELECTION}" | egrep "n"
if [ $? -eq 0 ]; then
	#now the NORMAL TESTS
	for ((i = 0; i < ${#NORMAL_TESTS[@]}; ++i)); do
		echo "Executing $i: ${NORMAL_TESTS[i]}"
		eval "./${NORMAL_TESTS[i]} ${TMP_FILE_OPTS}"
		NORMAL_TESTS_RESULT[i]=$?
		ls -1 /tmp/sserialize*
		if [ $? -eq 0 ]; then
			echo "tmp broken for $i"
			exit 1
		fi
	done
fi

echo "${TEST_SELECTION}" | egrep "s"
if [ $? -eq 0 ]; then
	#slow tests
	for ((i = 0; i < ${#SLOW_TESTS[@]}; ++i)); do
		echo "Executing $i: ${SLOW_TESTS[i]}"
		eval "./${SLOW_TESTS[i]} ${TMP_FILE_OPTS}"
		SLOW_TESTS_RESULT[i]=$?
		ls -1 /tmp/sserialize*
		if [ $? -eq 0 ]; then
			echo "tmp broken for $i"
			exit 1
		fi
	done
fi

#and now the result output

for ((i = 0; i < ${#FAST_TESTS_RESULT[@]}; ++i)); do
	if [ ${FAST_TESTS_RESULT[i]} -ne 0 ]; then
		echo "Test number $i = ${FAST_TESTS[i]} failed"
		TEST_FAILED=1
	fi
done

for ((i = 0; i < ${#NORMAL_TESTS_RESULT[@]}; ++i)); do
	if [ ${NORMAL_TESTS_RESULT[i]} -ne 0 ]; then
		echo "Test number $i = ${NORMAL_TESTS[i]} failed"
		TEST_FAILED=1
	fi
done

for ((i = 0; i < ${#SLOW_TESTS_RESULT[@]}; ++i)); do
	if [ ${SLOW_TESTS_RESULT[i]} -ne 0 ]; then
		echo "Test number $i = ${SLOW_TESTS[i]} failed"
		TEST_FAILED=1
	fi
done

if [ ${TEST_FAILED} -eq 1 ]; then
	exit 1
fi