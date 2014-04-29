#ifndef SSERIALIZE_GEO_STRINGS_ITEM_DB_H
#define SSERIALIZE_GEO_STRINGS_ITEM_DB_H
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/templated/GeoStringsItem.h>
#include <sserialize/Static/Array.h>
#include "StringsItemDB.h"
#include <sserialize/utility/ProgressInfo.h>


namespace sserialize {

template<typename ItemType>
class GeoStringsItemDBPrivate: public StringsItemDBPrivate<ItemType> {
public:
	typedef std::vector< spatial::GeoShape* > GeoShapeContainerType;
protected:
	typedef StringsItemDBPrivate<ItemType> MyParentClass;
private:
	GeoShapeContainerType m_geoShapes;
public:
	GeoStringsItemDBPrivate() : StringsItemDBPrivate<ItemType>() {}
	virtual ~GeoStringsItemDBPrivate() {
		for(GeoShapeContainerType::iterator it = m_geoShapes.begin(); it != m_geoShapes.end(); ++it) {
			delete *it;
		}
	}
	
	spatial::GeoRect rect() const {
		if (!m_geoShapes.size())
			return spatial::GeoRect();
		spatial::GeoRect rect;
		size_t i = 0;

		//find the first valid rect
		for(; i < m_geoShapes.size(); i++) {
			if (m_geoShapes[i]) {
				rect = m_geoShapes[i]->boundary();
				break;
			}
		}
		for(; i < m_geoShapes.size(); i++) {
			if (m_geoShapes[i])
				rect.enlarge(m_geoShapes[i]->boundary());
		}
		return rect;
	}
	
	unsigned int push_back(const std::deque<std::string> & strs, const ItemType & item, spatial::GeoShape * shape = 0) {
		unsigned int id = MyParentClass::push_back(strs, item);
		if (m_geoShapes.size() <= id)
			m_geoShapes.resize(id+1, 0);
		m_geoShapes[id] = shape;
		return id;
	}

	unsigned int push_back(const std::vector<std::string> & strs, const ItemType & item, spatial::GeoShape * shape = 0) {
		unsigned int id = MyParentClass::push_back(strs, item);
		if (m_geoShapes.size() <= id)
			m_geoShapes.resize(id+1, 0);
		m_geoShapes[id] = shape;
		return id;
	}
	
	bool setGeoShape(uint32_t itemPos, spatial::GeoShape * shape) {
		if (MyParentClass::size() < itemPos)
			return false;
		if (m_geoShapes.size() < itemPos) {
			m_geoShapes.resize(itemPos+1,0);
			m_geoShapes[itemPos] = shape;
		}
		else {
			using std::swap;
			swap(shape, m_geoShapes[itemPos]);
			delete shape;
		}
		return true;
	}
	
	spatial::GeoShape const * geoShape(uint32_t itemPos) const {
		if (m_geoShapes.size() < itemPos)
			return 0;
		return m_geoShapes[itemPos];
	}
	
	bool match(uint32_t itemPos, const spatial::GeoRect & boundary) {
		if (m_geoShapes.size() < itemPos)
			return false;
		return m_geoShapes.at(itemPos)->intersects(boundary);
	}
	
	/** Assigns item ids **/
	void reAssignItemIds(const std::map< uint32_t, uint32_t > & oldToNew) {
		GeoShapeContainerType shapes;
		shapes.resize(m_geoShapes.size());
		
		ProgressInfo info;
		info.begin(m_geoShapes.size(), "GeoStringsItem::reAssignItemIds");
		for(size_t oldId = 0; oldId < m_geoShapes.size(); ++oldId) {
			uint32_t newId = oldToNew.at(oldId);
			shapes[newId] = m_geoShapes[oldId];
			info(oldId);
		}
		info.end("GeoStringsItem::reAssignItemIds");
		m_geoShapes.swap(shapes);
	}

	
	void absorb(GeoStringsItemDBPrivate & other) {
		if (!m_geoShapes.size())
			m_geoShapes.swap(other.m_geoShapes);
		else
			m_geoShapes.insert(m_geoShapes.end(), other.m_geoShapes.begin(), other.m_geoShapes.end());
		other.m_geoShapes.clear();
		StringsItemDBPrivate<ItemType>::absorb(other);
	}
};

template<typename ItemType>
class GeoStringsItemDB: public StringsItemDB<ItemType> {
public:
	typedef GeoStringsItem< GeoStringsItemDB<ItemType> > Item;
protected:
	typedef GeoStringsItemDBPrivate<ItemType> MyPrivateClass;
	typedef StringsItemDB<ItemType> MyParentClass;

	GeoStringsItemDBPrivate<ItemType> * priv() const {
		return static_cast<GeoStringsItemDBPrivate<ItemType>*>( StringsItemDB<ItemType>::priv() );
	}
public:
	GeoStringsItemDB() : StringsItemDB<ItemType>( new MyPrivateClass() ) {}
	GeoStringsItemDB(GeoStringsItemDBPrivate<ItemType> * initial) : StringsItemDB<ItemType>(initial) {}
	GeoStringsItemDB(const GeoStringsItemDB<ItemType> & other) : MyParentClass(other) {}
	virtual ~GeoStringsItemDB() {}
	
	spatial::GeoRect rect() const {
		return priv()->rect();
	}

	unsigned int insert(const std::deque<std::string> & strs, const ItemType & item, spatial::GeoShape * shape = 0) {
		return priv()->push_back(strs, item, shape);
	}

	unsigned int insert(const std::string & strs, const ItemType & item, spatial::GeoShape * shape = 0) {
		return priv()->push_back(std::deque<std::string>(1, strs), item, shape);
	}

	unsigned int insert(const std::vector<std::string> & strs, const ItemType & item, spatial::GeoShape * shape = 0) {
		return priv()->push_back(strs, item, shape);
	}
	
	/** @param: shape will be an owning pointer: it get's deleted if this class instance gets deleted or if the items geoShape changes */
	bool setGeoShape(uint32_t itemPos, spatial::GeoShape * shape) {
		return priv()->setGeoShape(itemPos, shape);
	}
	
	spatial::GeoShape const * geoShape(uint32_t itemPos) const {
		return priv()->geoShape(itemPos);
	}
	
	bool match(uint32_t itemPos, const spatial::GeoRect & boundary) {
		return priv()->match(itemPos, boundary);
	}
	
	Item at(uint32_t pos) const {
		return Item(pos, *this);
	}
	
	/** Assigns item ids **/
	void reAssignItemIds(std::map< uint32_t, uint32_t > & oldToNew) {
		priv()->reAssignItemIds(oldToNew);
		MyParentClass::reAssignItemIds(oldToNew);
	}

	
	void absorb(GeoStringsItemDB & other) {
		priv()->absorb( *(other.priv()) );
	}

};


}//end namespace


template<typename ItemType>
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::GeoStringsItemDB<ItemType> & db) {
	sserialize::UByteArrayAdapter::OffsetType dO = destination.tellPutPtr();
	destination << static_cast<uint8_t>(0);
	destination << static_cast< const sserialize::StringsItemDB<ItemType>& >(db);
	sserialize::Static::ArrayCreator<sserialize::spatial::GeoShape> creator(destination);
	sserialize::ProgressInfo info;
	info.begin(db.items().size());
	for(size_t i = 0; i < db.size(); ++i) {
		creator.beginRawPut();
		const sserialize::spatial::GeoShape * shape = db.geoShape(i);
		if (shape)
			shape->appendWithTypeInfo( creator.rawPut() );
		creator.endRawPut();
		info(i, "GeoStringsItemDB<ItemType>::serialize::Shapes");
	}
	creator.flush();
	info.end("GeoStringsItemDB<ItemType>::serialize::Shapes");
	std::cout << "GeoStringsItemDB<ItemType>::getSizeInBytes()=" << destination.tellPutPtr()-dO << std::endl;
	return destination;
}

#endif