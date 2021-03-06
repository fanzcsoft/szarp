#ifndef __LOCATIONS_LOCATIONS_LIST_H__
#define __LOCATIONS_LOCATIONS_LIST_H__

#include <unordered_map>
#include <functional>

#include "location.h"
#include "protocol.h"

#include "utils/signals.h"
#include "utils/iterator.h"

class LocationsList {
	struct LocationEntry {
		std::string draw_name;
		std::string type;
		std::function<Location::ptr ()> generator;
	};

	typedef std::unordered_map<std::string,LocationEntry> GenetratorsMap;

public:
	LocationsList() {}
	virtual ~LocationsList() {}

	class iterator : public key_iterator<GenetratorsMap> {
		friend class LocationsList;
	public:
		iterator( const iterator& itr )
			: key_iterator<GenetratorsMap>(itr.itr) {}
		iterator( const GenetratorsMap::const_iterator& itr )
			: key_iterator<GenetratorsMap>(itr) {}
		const std::string& get_name() { return itr->second.draw_name; }
		const std::string& get_type() { return itr->second.type; }
	};

	iterator begin() const { return iterator(locations_generator.begin()); }
	iterator end  () const { return iterator(locations_generator.end  ()); }

	iterator find( const std::string& tag ) const
	{	return iterator(locations_generator.find(tag)); }

	template<class T,class... Args> void register_location(
			const std::string& tag ,
			const std::string& draw_name ,
			const std::string& type ,
			Args... args )
	{
		std::function<Location::ptr ()> f = 
			std::bind( &LocationsList::create_location<T,Args...> , this , tag , args... );
		locations_generator[ tag ] =
			LocationEntry { draw_name , type , f };

		emit_location_added( tag );
	}

	void remove_location( const std::string& tag )
	{
		if( !locations_generator.count(tag) )
			return;

		locations_generator.erase( tag );

		emit_location_removed( tag );
	}

	iterator remove_location( const iterator& itr )
	{
		auto tag = *itr;
		auto i = locations_generator.erase( itr.itr );

		emit_location_removed( tag );

		return iterator(i);
	}

	Location::ptr make_location( const std::string& tag )
	{
		auto itr = locations_generator.find(tag);
		return itr != locations_generator.end() ?
			itr->second.generator() :
			Location::ptr();
	}

	slot_connection on_location_added  ( const sig_string_slot& slot )
	{	return emit_location_added.connect( slot ); }
	slot_connection on_location_removed( const sig_string_slot& slot )
	{	return emit_location_removed.connect( slot ); }

private:
	/* Can be specialized to other types if needed (17/05/2014 08:55, jkotur) */
	template<class T,class... Args> Location::ptr create_location( const std::string& tag , Args... args )
	{
		return std::make_shared<T>( tag , args... );
	}

	GenetratorsMap locations_generator;

	sig_string emit_location_added;
	sig_string emit_location_removed;

};

#endif /* end of include guard: __LOCATIONS_LOCATIONS_LIST_H__ */

