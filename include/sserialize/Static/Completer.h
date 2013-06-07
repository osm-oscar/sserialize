#ifndef SSERIALIZE_STATIC_COMPLETER_H 
#define SSERIALIZE_STATIC_COMPLETER_H
#include <string>
#include <ostream>
#include <sserialize/utility/log.h>
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/Static/StringTable.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/completers/StringCompleter.h>
#include <sserialize/Static/StringCompleterPrivates.h>
#include "FlatGeneralizedTrie.h"
#include <sserialize/completers/StringCompleterPrivateStaticDB.h>
#include <sserialize/Static/FlatGSTStrIds.h>
#include <sserialize/completers/CompleterPrivate.h>
#include <sserialize/containers/ItemSetIterator.h>

namespace sserialize {
namespace Static {

template<class ItemType, class DataBaseType >
class Completer: public sserialize::CompleterPrivate<ItemType, DataBaseType> {
protected:
	typedef sserialize::CompleterPrivate<ItemType, DataBaseType> MyPrivateClass;
private:
	std::vector<UByteArrayAdapter> m_strCompleterData;
	UByteArrayAdapter m_indexData;
	UByteArrayAdapter m_dbData;
	UByteArrayAdapter m_stableData;
	bool m_engineStatus;

	int m_selectedCompleter;
	std::vector<sserialize::StringCompleter> m_strCompleters;
	Static::StringTable m_stable;
	DataBaseType m_itemDataBase;
	Static::ItemIndexStore m_indexStore;
protected:
	int8_t addStringCompleter(const UByteArrayAdapter & data);
	void clear() {
		m_selectedCompleter = -1;
		m_strCompleters = std::vector<sserialize::StringCompleter>();
		m_stable = Static::StringTable();
		m_itemDataBase = DataBaseType();
		m_indexStore = Static::ItemIndexStore();
	
		m_indexData = UByteArrayAdapter();
		m_dbData = UByteArrayAdapter();
		m_stableData = UByteArrayAdapter();
		m_engineStatus = false;
		m_strCompleterData= std::vector<UByteArrayAdapter>();
	}
protected:
	/** This just creates an itemset and registers all available completers */
	void createItemSet(ItemSet<ItemType, DataBaseType> & itemset, const std::string & query, SetOpTree::SotType type) {
		itemset = ItemSet<ItemType, DataBaseType>(query, getCompleter(), getItemDataBase(), type);
	}
	/** This just creates an itemsetiterator and registers all available completers */
	void createItemSetIterator(ItemSetIterator<ItemType, DataBaseType> & itemsetit, const std::string & query, SetOpTree::SotType type) {
		itemsetit = ItemSetIterator<ItemType, DataBaseType>(query, getCompleter(), getItemDataBase());
	}
	
	void createItemSet(ItemSet<ItemType, DataBaseType> & itemset, const std::string & query, const SetOpTree & setOpTree) {
		itemset = ItemSet<ItemType, DataBaseType>(query, getItemDataBase(), setOpTree);
	}
	
	void registerStringCompleter(ItemSet<ItemType, DataBaseType> & itemset) {
		itemset.registerStringCompleter(getCompleter());
	}
	
public:
	Completer();
	virtual ~Completer();
	/** Add a completer data, contains header to select completer type! */
	void addStringCompleterData(const sserialize::UByteArrayAdapter& data) { m_strCompleterData.push_back(data);}
	void setIndexData(const UByteArrayAdapter & data) { m_indexData = data; }
	void setDBData(const UByteArrayAdapter & data) { m_dbData = data; }
	void setStringTableData(const UByteArrayAdapter & data) { m_stableData = data; }
	bool energize();
	bool setCompleter(int id);
	///@return -1 if none selected
	int selectedCompleter() const { return m_selectedCompleter; }

public:
	inline UByteArrayAdapter getStringCompleterData() {
		if (m_selectedCompleter > 0)
			return m_strCompleterData[m_selectedCompleter-1];
		else
			return UByteArrayAdapter();
	}
	inline UByteArrayAdapter getIndexData() { return m_indexData;}
	inline UByteArrayAdapter getDBData() { return m_dbData;}
	inline UByteArrayAdapter getStringTableData() { return m_stableData;}
	inline std::size_t getCompletersSize() const { return m_strCompleters.size(); }
	inline const std::vector<sserialize::StringCompleter> & getCompleters() const { return m_strCompleters; }
	///@warning This will cause undefined behaviour if no completer is selected (you'll most probably get a segfault) 
	inline const sserialize::StringCompleter & getCompleter() const { return m_strCompleters[m_selectedCompleter];}
	inline const Static::StringTable & getStringTable() const { return m_stable; }
	inline const DataBaseType & getItemDataBase() const { return m_itemDataBase; }
	inline const Static::ItemIndexStore & getIndexStore() const { return m_indexStore;}
	inline std::vector<sserialize::StringCompleter> & getStringCompleters() { return m_strCompleters; }
	inline sserialize::StringCompleter & getStringCompleter() { return m_strCompleters[m_selectedCompleter];}
	inline Static::StringTable & getStringTable() { return m_stable; }
	inline DataBaseType & getItemDataBase() { return m_itemDataBase; }
	inline Static::ItemIndexStore & getIndexStore() { return m_indexStore;}
	inline bool engineStatus() { return m_engineStatus;}
	std::ostream& printStats(std::ostream & out) const;

public:
	virtual ItemSet<ItemType, DataBaseType> complete(const std::string& query);
	sserialize::ItemSetIterator<ItemType, DataBaseType> partialComplete(const std::string& query);
};

template<class ItemType, class DataBaseType>
Completer<ItemType, DataBaseType>::Completer() :
m_engineStatus(false),
m_selectedCompleter(-1)
{}

template<class ItemType, class DataBaseType>
Completer<ItemType, DataBaseType>::~Completer() {}

template<class ItemType, class DataBaseType>
bool
Completer<ItemType, DataBaseType>::setCompleter(int id) {
	if (id >= 0 && m_strCompleters.size() > static_cast<std::size_t>(id) ) {
		m_selectedCompleter = id;
		return true;
	}
	return false;
}

template<class ItemType, class DataBaseType>
ItemSet<ItemType, DataBaseType> Completer<ItemType, DataBaseType>::complete(const std::string & query) {
	ItemSet<ItemType, DataBaseType> itemSet(query, getCompleter(), getItemDataBase(), sserialize::SetOpTree::SOT_COMPLEX);
	itemSet.execute();
	return itemSet;
}

template<class ItemType, class DataBaseType>
ItemSetIterator<ItemType, DataBaseType> Completer<ItemType, DataBaseType>::partialComplete(const std::string & query) {
	ItemSetIterator<ItemType, DataBaseType> itemSet(query, getCompleter(), getItemDataBase());
	itemSet.execute();
	return itemSet;
}

template<class ItemType, class DataBaseType>
bool Completer<ItemType, DataBaseType>::energize() {
	if (m_indexData.size() == 0 || m_stableData.size() == 0 || m_dbData.size() == 0) {
		osmfindlog::err("sserialize::Completer::energize", "At least one needed data array is empty");
		return false;
	}
	m_indexStore = Static::ItemIndexStore(m_indexData);
	m_stable = Static::StringTable(m_stableData); 
	m_itemDataBase = DataBaseType(m_dbData, m_stable);
	m_strCompleters.push_back(sserialize::StringCompleter(new StringCompleterPrivateStaticDB<DataBaseType>(m_itemDataBase)) );
	m_engineStatus = true;
	
	for(size_t i = 0; i < m_strCompleterData.size(); i++) {
		int8_t id =  addStringCompleter(m_strCompleterData[i]);
		if (id > 0)
			m_selectedCompleter = id;
		else
			osmfindlog::err("sserialize::Completer::energize", "Failed to init a completer");
	}
	
	return true;
}

template<class ItemType, class DataBaseType>
int8_t Completer<ItemType, DataBaseType>::addStringCompleter(const UByteArrayAdapter & data) {
	uint8_t id = m_strCompleters.size();
	uint8_t type = data[1];
	switch (type) {
	case (Static::StringCompleter::T_TRIE):
		m_strCompleters.push_back(sserialize::StringCompleter(new Static::GeneralizedTrie(data+2, getIndexStore())) );
		break;
	case (Static::StringCompleter::T_FLAT_TRIE):
		m_strCompleters.push_back(sserialize::StringCompleter(new Static::FlatGST (data+2, getIndexStore())) );
		break;
	case (Static::StringCompleter::T_FLAT_TRIE_STRIDS):
		m_strCompleters.push_back(
			sserialize::StringCompleter(
				new sserialize::Static::FlatGSTStrIds<DataBaseType>( data+2, getIndexStore(), m_itemDataBase)
			)
		);
		break;
	default:
		return -1;
	}
	return id;
}

template<class ItemType, class DataBaseType>
std::ostream& Completer<ItemType, DataBaseType>::printStats(std::ostream & out) const {
	m_stable.printStats(out);
	m_itemDataBase.printStats(out);
	m_indexStore.printStats(out);
	for(size_t i = 0; i < m_strCompleters.size(); i++) {
		m_strCompleters[i].printStats(out);
	}
	return out;
}

}}//end namespace

#endif