#ifndef SSERIALIZE_STATIC_FILE_BASED_COMPLETER_GEO_H
#define SSERIALIZE_STATIC_FILE_BASED_COMPLETER_GEO_H
#include <sserialize/Static/FileBasedCompleter.h>
#include <sserialize/completers/GeoCompleter.h>
#include <sserialize/Static/ItemGeoGrid.h>
#include <sserialize/completers/GeoCompleterPrivateProxy.h>
#include <sserialize/spatial/GeoConstraintSetOpTreeEF.h>
#include <sserialize/utility/types.h>

namespace sserialize {
namespace Static {

/** This class abstracts completion. Just put in the correct data and you're good to go.
  * @DataBaseType: This class has to support all functions that the GeoStringsItemDB supports */
template<class ItemType, class DataBaseType >
class CompleterGeo: public sserialize::Static::Completer<ItemType, DataBaseType> {
protected:
	typedef sserialize::Static::Completer<ItemType, DataBaseType> MyParentClass;
private:
	std::vector<UByteArrayAdapter> m_geoCompleterData;
	std::vector<sserialize::GeoCompleter> m_geoCompleters;
	uint8_t m_selectedGeoCompleter;
protected:
	/** currently this only support the ItemGeoGrid, this has to be called after init */
	int8_t addGeoCompleter(const UByteArrayAdapter & data) {
		if (!data.size() || !MyParentClass::engineStatus())
			return -1;
		int8_t id = m_geoCompleters.size();

		GeoCompleterPrivateProxy< sserialize::Static::spatial::ItemGeoGrid<DataBaseType> > * gcp = 
			new GeoCompleterPrivateProxy< Static::spatial::ItemGeoGrid<DataBaseType> >(
				sserialize::Static::spatial::ItemGeoGrid<DataBaseType>(data, MyParentClass::getItemDataBase(), MyParentClass::getIndexStore())
				);
		m_geoCompleters.push_back( sserialize::GeoCompleter(gcp) );
		return id;
	}
	void createItemSet(ItemSet<ItemType, DataBaseType> & itemset, const std::string & query, SetOpTree::SotType type) {
		MyParentClass::createItemSet(itemset, query, type);
		itemset.registerSelectableOpFilter( new sserialize::spatial::GeoConstraintSetOpTreeSF<sserialize::GeoCompleter>( getGeoCompleter()) );
	}
	void createItemSetIterator(ItemSetIterator<ItemType, DataBaseType> & itemsetit, const std::string & query, SetOpTree::SotType type) {
		MyParentClass::createItemSetIterator(itemsetit, query, type);
		itemsetit.registerSelectableOpFilter( new sserialize::spatial::GeoConstraintSetOpTreeSF<sserialize::GeoCompleter>( getGeoCompleter()) );
	}
	
	void clear() {
		m_geoCompleterData = std::vector<UByteArrayAdapter>();
		m_geoCompleters = std::vector<sserialize::GeoCompleter>();
		m_selectedGeoCompleter = 0;
		MyParentClass::clear();
	}
	
public:
	CompleterGeo() : MyParentClass(), m_selectedGeoCompleter(0) {}
	virtual ~CompleterGeo() {}
	bool energize() {
		bool ok = MyParentClass::energize();
		if (!ok)
			return ok;
		GeoCompleterPrivateProxy< DataBaseType > * gcp = new GeoCompleterPrivateProxy< DataBaseType >(MyParentClass::getItemDataBase() );
		m_geoCompleters.push_back( sserialize::GeoCompleter(gcp) );
		
		for(size_t i = 0; i < m_geoCompleterData.size(); i++) {
			int8_t id =  addGeoCompleter(m_geoCompleterData[i]);
			if (id > 0)
				m_selectedGeoCompleter = id;
			else {
				osmfindlog::err("sserialize::GeoCompleter::energize", "Failed to init a geo completer");
				ok = false;
			}
		}
		return ok;
	}
	bool setGeoCompleter(uint8_t id) {
		if (m_geoCompleters.size() <= id)
			return false;
		m_selectedGeoCompleter = id;
		return true;
	}
	
	uint8_t selectedGeoCompleter() {
		return m_selectedGeoCompleter;
	}
	
	void addGeoCompleterData(const sserialize::UByteArrayAdapter& data) { m_geoCompleterData.push_back(data);}
	const GeoCompleter & getGeoCompleter() const { return m_geoCompleters[m_selectedGeoCompleter];}
	const std::vector<sserialize::GeoCompleter> & getGeoCompleters() const { return m_geoCompleters; }
	std::ostream& printStats(std::ostream & out) const {
		MyParentClass::printStats(out);
		for(size_t i = 0; i < m_geoCompleters.size(); i++) {
			m_geoCompleters[i].printStats(out);
		}
		return out;

	}

};

template<class ItemType, class DataBaseType, class CompleterBase = sserialize::Static::CompleterGeo<ItemType, DataBaseType> >
class FileBasedCompleterGeo: public FileBasedCompleter<ItemType, DataBaseType, CompleterBase> {
protected:
	typedef FileBasedCompleter<ItemType, DataBaseType, CompleterBase> MyParentClass;
private:
	std::vector<std::string> m_geoCompleterFileNames;
	std::vector<bool> m_geoCompleterFilesCompression;
protected:
	bool openAll() {
		for(size_t i = 0; i < m_geoCompleterFileNames.size(); ++i)  {
			UByteArrayAdapter adap(UByteArrayAdapter::openRo(m_geoCompleterFileNames[i], m_geoCompleterFilesCompression[i], MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT));
		
			if (adap.size()) {
				CompleterBase::addGeoCompleterData(adap);
			}
			else {
				osmfindlog::err("sserialize::FileBasedCompleterGeo::openAll", "Failed to open geo completer file: " + m_geoCompleterFileNames[i]);
			}
		}
		return true;
	}
	
	void closeAll() {
		MyParentClass::clear();
		MyParentClass::closeAll();
	}
public:
	FileBasedCompleterGeo() : MyParentClass() {}
	virtual ~FileBasedCompleterGeo() {}
	/** @return true, if file exisists */
	bool addGeoCompleterFile(const std::string & filename, bool compressed) {
		if (MmappedFile::fileExists(filename)) {
			m_geoCompleterFileNames.push_back(filename);
			m_geoCompleterFilesCompression.push_back(compressed);
			return true;
		}
		else {
			osmfindlog::err("FileBasedCompleterGeo::addGeoCompleterFile", "File " + filename + " is not available"); 
		}
		return false;
	}

	bool energize() {
		if (!openAll())
			return false;
		return MyParentClass::energize();
	}

};

}}//end namespace


#endif