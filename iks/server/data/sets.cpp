#include "sets.h"

#include <locale>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bp = boost::property_tree;
namespace bm = boost::multi_index;
using boost::format;

#include <liblog.h>

#include "utils/ptree.h"

std::shared_ptr<const Set> Sets::get_set( const std::string& name ) const
{
	auto itr = bm::get<set_name>(sets).find(name);
	return itr == bm::get<set_name>(sets).end() ? std::shared_ptr<Set>() : itr->set;
}

void Sets::update_set( const Set& s , const std::string& old_name )
{
	std::string on = old_name.empty() ? s.get_name() : old_name;

	auto& sname = bm::get<set_name>(sets);

	if( !has_set(on) && !s.empty() ) {
		/** Put in order if got one otherwise at the end */
		auto order =
			s.has_order() ?
				s.get_order() :
				bm::get<set_id>(sets).rbegin()->order + 1;
		auto ps = std::make_shared<Set>( s );
		sname.insert( SetEntry { s.get_name() , order , ps } );
		emit_set_updated( on , ps ); /** New set */
		return;
	}

	auto itr = sname.find(on);

	if( s.empty() ) {
		/** Set removed */
		sname.erase( itr );
		emit_set_updated( on , std::shared_ptr<Set>() );
		return;
	}

	if( *itr->set != s ) {
		if( s.get_name() != on )
			sname.erase( itr );

		/** New order if set -- otherwise old order */
		auto order = s.has_order() ? s.get_order() : itr->order;

		auto ps = std::make_shared<Set>( s );
		sname.insert( SetEntry { s.get_name() , order , ps } );

		/** Set updated */
		emit_set_updated( on , ps );
	}
}

void Sets::from_params_file( const std::string& path ) throw(xml_parse_error)
{
	bp::ptree sets_doc;
	bp::read_xml(path, sets_doc, bp::xml_parser::trim_whitespace|bp::xml_parser::no_comments );

	from_params_xml( sets_doc );
}

void Sets::from_params_xml( boost::property_tree::ptree& ptree  ) throw(xml_parse_error)
{
	std::unordered_map<std::string,std::pair<double,bp::ptree>> sets_map;
	double max_order = 0;
	double unordered = 0;

	fold_xmlattr( ptree );

	ptree_foreach_parent( ptree , [&] ( bp::ptree::value_type& d , bp::ptree& parent ) {
		if( d.first == "draw" ) {
			auto name = d.second.get<std::string>("@title");
			bp::ptree child;
			child.put("@name",parent.get<std::string>("@name"));
			child.put("@min",d.second.get<std::string>("@min"));
			child.put("@max",d.second.get<std::string>("@max"));
			auto color = d.second.get_optional<std::string>("@color");
			if( color ) child.put("@graph_color",*color);
			auto order = d.second.get_optional<std::string>("@order");
			if( order ) child.put("@order",*order);
			if( !sets_map.count(name) )
				/** If set no know default order to NaN */
				sets_map[name].first = --unordered;
			auto seto   = d.second.get_optional<std::string>("@prior");
			if( seto  ) {
				try {
					auto order = boost::lexical_cast<double>(*seto);
					sets_map[name].first = order;
					max_order = std::max(order,max_order);
				} catch( boost::bad_lexical_cast& e ) {
					sz_log(0, "Invalid prior in set %s", name.c_str() );
				}
			}
			sets_map[name].second.push_back( std::make_pair( "" , child ) );
		}
	} );

	for( auto& c : sets_map )
	{
		bp::ptree ptree;
		ptree.put("@name",c.first);
		ptree.put_child("params",c.second.second);

		auto order = c.second.first;
		order = order >=0 ? order : max_order - order ;

		Set s;
		s.from_json( ptree );
		s.set_order( order );
		update_set( s );
	}
}

