#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <sserialize/containers/ItemIndexFactory.h>
#include <random>


std::default_random_engine random_engine(0);

std::vector<uint32_t> createNumbersSet(uint32_t count, uint32_t max) {
	std::set<uint32_t> ret;
	//Fill the first
	std::uniform_int_distribution<int> d(1, max);
	
	for(uint32_t i(0); i < count; ++i) {
		ret.insert( d(random_engine) );
	}
	return std::vector<uint32_t>(ret.begin(), ret.end());
}

static std::vector<uint32_t> merge(const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
	std::vector<uint32_t> result;
	result.resize(a.size()+b.size());
	auto aIt(a.cbegin());
	auto aEnd(a.cend());
	auto bIt(b.cbegin());
	auto bEnd(b.cend());
	auto rIt(result.begin());
	while (aIt != aEnd && bIt != bEnd) {
		if (*aIt == *bIt) {
			*rIt = *aIt;
			++aIt;
			++bIt;
		}
		else if (*aIt < *bIt) {
			*rIt = *aIt;
			++aIt;
		}
		else { //bItemId is smaller
			*rIt = *bIt;
			++bIt;
		}
		++rIt;
	}
	for(; aIt != aEnd; ++aIt, ++rIt) {
		*rIt = *aIt;
	}
	for(; bIt != bEnd; ++bIt, ++rIt) {
		*rIt = *bIt;
	}
	result.resize(rIt-result.begin());
	return result;
}

enum IndexType {
	__IT_FIRST_OWN=2*sserialize::ItemIndex::__T_LAST_ENTRY,
	IT_VECTOR_TREE_MERGE=__IT_FIRST_OWN,
	IT_VECTOR_SET_MERGE=2*IT_VECTOR_TREE_MERGE,
	IT_VECTOR_HEAP_MERGE=2*IT_VECTOR_SET_MERGE,
	__IT_MERGE_WITH_VECTOR=2*IT_VECTOR_HEAP_MERGE,
	__IT_MERGE_WITH_HEAP=2*__IT_MERGE_WITH_VECTOR
};

struct TestDataBase {
	using meas_res = std::chrono::milliseconds;
	std::vector<meas_res> times;
	
	 virtual ~TestDataBase() {}
	
	void run() {
		auto start = std::chrono::high_resolution_clock::now();
		run_merge();
		auto stop = std::chrono::high_resolution_clock::now();
		times.push_back( std::chrono::duration_cast<meas_res>(stop-start) );
	}
	
	virtual void run_merge() = 0;
	virtual std::string name() const = 0;
	virtual int type() const = 0;
	
	meas_res meanTime() {
		return sserialize::statistics::mean(times.begin(), times.end(), meas_res(0));
	}
};

struct ItemIndexTestData: TestDataBase {
	std::vector<sserialize::ItemIndex> buckets;
	sserialize::ItemIndex::Types t;
	ItemIndexTestData(const std::vector<std::vector<uint32_t>> & src, sserialize::ItemIndex::Types t) : t(t) {
		buckets.reserve(src.size());
		for(const auto & x : src) {
			buckets.emplace_back( sserialize::ItemIndexFactory::create(x, t) );
		}
	}
	virtual ~ItemIndexTestData() override {}
	virtual void run_merge() override {
		sserialize::ItemIndex result = sserialize::ItemIndex::unite(buckets);
		std::cout << name() << "::result-size: " << result.size() << std::endl;
	}
	virtual std::string name() const {
		return "ItemIndex::" + sserialize::to_string(t);
	}
	virtual int type() const {
		return t;
	}
};

struct ItemIndexMergeWithVectorTestData: ItemIndexTestData {
	ItemIndexMergeWithVectorTestData(const std::vector<std::vector<uint32_t>> & src, sserialize::ItemIndex::Types t) :
	ItemIndexTestData(src, t)
	{}
	virtual ~ItemIndexMergeWithVectorTestData() override {}
	virtual void run_merge() override {
		std::vector<uint32_t> result = sserialize::treeReduceMap<std::vector<sserialize::ItemIndex>::const_iterator, std::vector<uint32_t> >(
			buckets.cbegin(), buckets.cend(),
			[](const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
				return merge(a, b);
			},
			[](const sserialize::ItemIndex & a) {
				return a.toVector();
			}
		);
		std::cout << name() << "::result-size: " << result.size() << std::endl;
	}
	virtual std::string name() const {
		return "ItemIndex-with-vector::" + sserialize::to_string(t);
	}
	virtual int type() const {
		return ItemIndexTestData::type() | __IT_MERGE_WITH_VECTOR;
	}
};

struct ItemIndexHeapMergeTestData: ItemIndexTestData {
	struct HeapElement {
		using iterator = sserialize::ItemIndex::const_iterator;
		iterator it;
		iterator end;
		HeapElement(const iterator & it, const iterator & end) : it(it), end(end) {}
		bool operator<(const HeapElement & other) const {
			return *it > *other.it; //prio-queue is a max heap, but we need a min-heap!
		}
		uint32_t value() const { return *it; }
		bool next() {
			if (it != end) {
				++it;
				return it != end;
			}
			return false;
		}
	};

	ItemIndexHeapMergeTestData(const std::vector<std::vector<uint32_t>> & src, sserialize::ItemIndex::Types t) :
	ItemIndexTestData(src, t)
	{}
	virtual ~ItemIndexHeapMergeTestData() override {}
	virtual void run_merge() override {
		std::vector<uint32_t> result;
		std::priority_queue<HeapElement> queue;
		for(const auto & x : buckets) {
			if (x.size()) {
				queue.emplace(x.cbegin(), x.cend());
			}
		}
		if (queue.empty()){
			return;
		}
		
		//extract first element, this way we don't have to check whether result is empty
		{
			HeapElement top = queue.top();
			queue.pop();
			result.emplace_back( top.value() );
			if (top.next()) {
				queue.push(top);
			}
		}
		while(!queue.empty()) {
			HeapElement top = queue.top();
			queue.pop();
			if (result.back() != top.value()) {
				result.emplace_back( top.value() );
			}
			if (top.next()) {
				queue.push(top);
			}
		}
		std::cout << name() << "::result-size: " << result.size() << std::endl;
	}
	virtual std::string name() const {
		return "ItemIndex-with-heap::" + sserialize::to_string(t);
	}
	virtual int type() const {
		return ItemIndexTestData::type() | __IT_MERGE_WITH_HEAP;
	}
};

struct VectorTreeMergeTestData: TestDataBase {
	const std::vector< std::vector<uint32_t> > * buckets;
	VectorTreeMergeTestData(const std::vector< std::vector<uint32_t> > & src) : buckets(&src) {}
	virtual ~VectorTreeMergeTestData() override {}
	
	virtual void run_merge() override {
		auto result = sserialize::treeReduce(
			buckets->cbegin(), buckets->cend(),
			[](const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
				return merge(a, b);
			}
		);
		std::cout << name() << "::result-size: " << result.size() << std::endl;
	}
	virtual std::string name() const {
		return "std::vector::tree-merge";
	}
	virtual int type() const {
		return IT_VECTOR_TREE_MERGE;
	}
};

struct VectorSetMergeTestData: TestDataBase {
	const std::vector< std::vector<uint32_t> > * buckets;
	VectorSetMergeTestData(const std::vector< std::vector<uint32_t> > & src) : buckets(&src) {}
	virtual ~VectorSetMergeTestData() override {}
	virtual void run_merge() override {
		std::set<uint32_t> result;
		for(const auto & x : *buckets) {
			result.insert(x.cbegin(), x.cend());
		}
		std::cout << name() << "::result-size: " << result.size() << std::endl;
	}
	virtual std::string name() const {
		return "std::vector::set-merge";
	}
	virtual int type() const {
		return IT_VECTOR_SET_MERGE;
	}
};

struct VectorHeapMergeTestData: TestDataBase {
	struct HeapElement {
		std::vector<uint32_t>::const_iterator it;
		std::vector<uint32_t>::const_iterator end;
		HeapElement(const std::vector<uint32_t>::const_iterator & it, const std::vector<uint32_t>::const_iterator & end) : it(it), end(end) {}
		bool operator<(const HeapElement & other) const {
			return *it > *other.it; //prio-queue is a max heap, but we need a min-heap!
		}
		uint32_t value() const { return *it; }
		bool next() {
			++it;
			return it < end;
		}
	};

	const std::vector< std::vector<uint32_t> > * buckets;
	VectorHeapMergeTestData(const std::vector< std::vector<uint32_t> > & src) : buckets(&src) {}
	virtual ~VectorHeapMergeTestData() override {}
	
	virtual void run_merge() override {
		std::vector<uint32_t> result;
		std::priority_queue<HeapElement> queue;
		for(const auto & x : *buckets) {
			if (x.size()) {
				queue.emplace(x.cbegin(), x.cend());
			}
		}
		if (queue.empty()){
			return;
		}
		
		//extract first element, this way we don't have to check whether result is empty
		{
			HeapElement top = queue.top();
			queue.pop();
			result.emplace_back( top.value() );
			if (top.next()) {
				queue.push(top);
			}
		}
		while(!queue.empty()) {
			HeapElement top = queue.top();
			queue.pop();
			if (result.back() != top.value()) {
				result.emplace_back( top.value() );
			}
			if (top.next()) {
				queue.push(top);
			}
		}
		std::cout << name() << "::result-size: " << result.size() << std::endl;
	}
	virtual std::string name() const {
		return "std::vector::heap-merge";
	}
	virtual int type() const {
		return IT_VECTOR_HEAP_MERGE;
	}
};

struct TestData {
	std::vector< std::vector<uint32_t> > source;
	std::vector<uint32_t> result;
	uint64_t treeMergeComparisonCount;
	std::map<int, std::unique_ptr<TestDataBase> > data;
	
	void init(uint32_t num_buckets, uint32_t bucket_fill) {
		source.reserve(num_buckets);
		for(uint32_t i(0); i < num_buckets; ++i) {
			source.emplace_back( createNumbersSet(bucket_fill, sserialize::narrow_check<uint32_t>(num_buckets*bucket_fill)) );
		}
		
		auto mergef = [this](const std::vector<uint32_t> & a, const std::vector<uint32_t> & b) {
			std::vector<uint32_t> result;
			auto aIt(a.cbegin());
			auto aEnd(a.cend());
			auto bIt(b.cbegin());
			auto bEnd(b.cend());
			while (aIt != aEnd && bIt != bEnd) {
				if (*aIt == *bIt) {
					result.emplace_back(*aIt);
					treeMergeComparisonCount += 1;
					++aIt;
					++bIt;
				}
				else if (*aIt < *bIt) {
					result.emplace_back(*aIt);
					treeMergeComparisonCount += 2;
					++aIt;
				}
				else { //bItemId is smaller
					result.emplace_back(*bIt);
					treeMergeComparisonCount += 3;
					++bIt;
				}
			}
			result.insert(result.end(), aIt, aEnd);
			result.insert(result.end(), bIt, bEnd);
			return result;
		};
		result = sserialize::treeReduce(source.cbegin(), source.cend(), mergef);
		std::cout << "initial result size: " << result.size() << std::endl;
	}
	
	void add_test(int t) {
		if (data.count(t)) {
			return;
		}
		if (t < __IT_FIRST_OWN) {
			data[t].reset(new ItemIndexTestData(source, (sserialize::ItemIndex::Types)t) );
		}
		else if (__IT_MERGE_WITH_VECTOR & t) {
			data[t].reset(new ItemIndexMergeWithVectorTestData(source, (sserialize::ItemIndex::Types)(t & ~(__IT_MERGE_WITH_VECTOR))));
		}
		else if (__IT_MERGE_WITH_HEAP & t) {
			data[t].reset(new ItemIndexHeapMergeTestData(source, (sserialize::ItemIndex::Types)(t & ~(__IT_MERGE_WITH_HEAP))));
		}
		else {
			switch (t) {
			case IT_VECTOR_TREE_MERGE:
				data[t].reset( new VectorTreeMergeTestData(source) );
				break;
			case IT_VECTOR_SET_MERGE:
				data[t].reset( new VectorSetMergeTestData(source) );
				break;
			case IT_VECTOR_HEAP_MERGE:
				data[t].reset( new VectorHeapMergeTestData(source) );
				break;
			default:
				break;
			}
		}
	}
	
	void run(uint32_t count = 1) {
		for(uint32_t i(0); i < count; ++i) {
			for(auto & x : data) {
				x.second->run();
			}
		}
	}
	
	void pretty_results(std::ostream & out) {
		out << "Bucket count: " << source.size() << '\n';
		out << "Bucket fill: " << (source.size() ? source.front().size() : std::size_t(0)) << '\n';
		for(const auto & x : data) {
			out << x.second->name() << ": " << x.second->meanTime().count() << '\n';
		}
	}
	
	void plot_header(std::ostream & out) {
		out << "bcount;bfill";
		for(const auto & x : data) {
			out << ';' << x.second->name();
		}
	}
	
	void plot_results(std::ostream & out) {
		out << source.size() << ';';
		out << (source.size() ? source.front().size() : std::size_t(0)) << ';';
		
		for(const auto & x : data) {
			out << ';' << x.second->meanTime().count();
		}
	}
};

struct Config {
	Config(uint32_t num_buckets, uint32_t bucket_fill) : num_buckets(num_buckets), bucket_fill(bucket_fill) {}
	uint32_t num_buckets;
	uint32_t bucket_fill;
	std::vector<IndexType> tests;
};

void printHelp() {
	std::cout << "\nNeed testCount bucketCount fillCount [bucketCount fillCount ... ]" << std::endl;
}

int main(int argc, char ** argv) {
	uint32_t testCount;
	std::vector<Config> cfgs;
	std::vector<int> types({
		IT_VECTOR_TREE_MERGE, IT_VECTOR_SET_MERGE, IT_VECTOR_HEAP_MERGE,
		sserialize::ItemIndex::T_NATIVE,
		sserialize::ItemIndex::T_WAH,
// 		sserialize::ItemIndex::T_ELIAS_FANO,
		sserialize::ItemIndex::T_RLE_DE,
		sserialize::ItemIndex::T_NATIVE | __IT_MERGE_WITH_VECTOR,
		sserialize::ItemIndex::T_WAH | __IT_MERGE_WITH_VECTOR,
		sserialize::ItemIndex::T_RLE_DE | __IT_MERGE_WITH_VECTOR,
		sserialize::ItemIndex::T_NATIVE | __IT_MERGE_WITH_HEAP,
		sserialize::ItemIndex::T_WAH | __IT_MERGE_WITH_HEAP,
		sserialize::ItemIndex::T_RLE_DE | __IT_MERGE_WITH_HEAP
	});
	
	if (argc < 3 || argc % 2 != 0) {
		printHelp();
		return -1;
	}
	
	testCount = ::atoi(argv[1]);
	for(int i(2); i < argc; i += 2) {
		cfgs.emplace_back(
			::atoi(argv[i]),
			::atoi(argv[i+1])
		);
	}
	
	for(const Config & c : cfgs) {
		TestData td;
		td.init(c.num_buckets, c.bucket_fill);
		for(int t : types) {
			td.add_test(t);
		}
		td.run(testCount);
		td.pretty_results(std::cout);
	}
	
	return 0;
}
