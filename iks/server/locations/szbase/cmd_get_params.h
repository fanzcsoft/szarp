#ifndef __SERVER_CMD_GET_PARAMS_H__
#define __SERVER_CMD_GET_PARAMS_H__

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "locations/command.h"
#include "data/vars.h"
#include "utils/ptree.h"

class GetParamsRcv : public Command {
public:
	GetParamsRcv( Vars& vars )
		: vars(vars)
	{
		set_next( std::bind(&GetParamsRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetParamsRcv()
	{
	}

	void parse_command( const std::string& data )
	{
		namespace bp = boost::property_tree;

		std::stringstream ss(data);
		bp::ptree json;

		try {
			bp::json_parser::read_json( ss , json );
		} catch( const bp::ptree_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		if( json.count("set") ) {
			auto ps = vars.get_sets().get_set(json.get<std::string>("set"));
			if( ps )
				send_params<Set,std::string>( *ps ,
					[]( const std::string& p ) { return p; } );
			else
				fail( ErrorCodes::unknown_set );
		} else if( json.count("params") ) {
			send_params<bp::ptree,bp::ptree::value_type>( json.get_child("params") ,
				[]( const bp::ptree::value_type& p ) { return p.second.get<std::string>(""); } );
		} else 
			fail( ErrorCodes::ill_formed );
	}

	template<class Container,class Key> void send_params(
			const Container& container ,
			const std::function<const std::string& (const Key&)>& get )
	{
		namespace bp = boost::property_tree;

		bp::ptree out;

		for( auto& p : container )
		{
			std::string s( get(p) );

			auto pp = vars.get_params().get_param( s );

			if( !pp ) {
				std::cerr << "Invalid param " << s << " requested" << std::endl;
				fail( ErrorCodes::unknown_param );
				return;
			}

			out.add_child( s , pp->get_ptree() );
		}

		apply( ptree_to_json( out , false ) );
	}


protected:
	Vars& vars;
};

#endif /* end of include guard: __SERVER_CMD_GET_PARAMS_H__ */

