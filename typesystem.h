#pragma once

#include <string>
#include <map>

#include <tree/property.h>

#include "types.h"
#include "value.h"

namespace treescript
{



template <class from, class to>
value_base_sp promote(property_base *operand, std::function<to(from)> op)
{
	auto typed = dynamic_cast<property<from> *>(operand);
	if(typed == nullptr)
	{
		return nullptr;
	}
	return value_base_sp(new value<to>(op(typed->get_value())));
}


template <class to>
void set_val(property_base *operand, value_base_sp v)
{
	auto typed = dynamic_cast<property<to> *>(operand);
	auto val = dynamic_cast<value<to> *>(v.get());
	if(typed == nullptr || val == nullptr)
	{
		return;
	}
	typed->set_value(val->get_value());
}




class typesystem
{
public:
	typesystem()
	{
		init();
	}
	
	
	std::string get_common_type(std::string left, std::string right)
	{
		if(left == right)
		{
			return left;
		}
		
		promotion_map_t::iterator it = promotion_map.find(left);
		if(it == promotion_map.end())
		{
			return "";
		}
		if(it->second.find(right) == it->second.end())
		{
			return "";
		}
		return promotion_map[left][right];
	}
	
	value_base_sp promote_to(property_base *val, std::string type)
	{
		if(promoters.find(val->get_type()) == promoters.end())
		{
			return nullptr;
		}
		if(promoters[val->get_type()].find(type) == promoters[val->get_type()].end())
		{
			return nullptr;
		}
		return promoters[val->get_type()][type](val);
	}
	
	void convert(property_base *from, property_base *to)
	{
		if(from->get_type() == to->get_type())
		{
			to->set_value(from);
		}
		else
		{
			set_value(to, promote_to(from, to->get_type()));
		}
	}
	
	void set_value(property_base *operand, value_base_sp v)
	{
		auto it = setter_map.find(operand->get_type());
		if(it == setter_map.end())
		{
			return;
		}
		auto &s = it->second;
		s(operand, v);
	}
	
	bool can_convert(std::string from, std::string to)
	{
		if(promoters.find(from) == promoters.end())
		{
			return false;
		}
		if(promoters[from].find(to) == promoters[from].end())
		{
			return false;
		}
		
		return true;
	}
	
	bool can_set(std::string type)
	{
		if(setter_map.find(type) == setter_map.end())
		{
			return false;
		}
		return true;
	}
	
	void init()
	{
		promotion_map[typeid(real_type).name()][typeid(int_type).name()] = typeid(real_type).name();
		promotion_map[typeid(int_type).name()][typeid(real_type).name()] = typeid(real_type).name();

		promotion_map[typeid(bool_type).name()][typeid(int_type).name()] = typeid(int_type).name();
		promotion_map[typeid(int_type).name()][typeid(bool_type).name()] = typeid(int_type).name();

		promotion_map[typeid(real_type).name()][typeid(bool_type).name()] = typeid(real_type).name();
		promotion_map[typeid(bool_type).name()][typeid(real_type).name()] = typeid(real_type).name();

		promoters[typeid(bool_type).name()][typeid(int_type).name()] = [](property_base * op){return promote<bool_type, int_type>(op, [](bool_type val){return ((val == true) ? 1 : 0);});};
		promoters[typeid(int_type).name()][typeid(real_type).name()] = [](property_base * op){return promote<int_type, real_type>(op, [](int_type val){return real_type(val);});};
		promoters[typeid(bool_type).name()][typeid(real_type).name()] = [](property_base * op){return promote<bool_type, real_type>(op, [](bool_type val){return ((val == true) ? 1.0 : 0.0);});};
		
		setter_map[typeid(bool_type).name()] = set_val<bool_type>;
		setter_map[typeid(int_type).name()] = set_val<int_type>;
		setter_map[typeid(real_type).name()] = set_val<real_type>;
	}
	
private:
	// to map two types TA and TB to a common type TC
	typedef std::map<std::string, std::string> inner_promotion_map_t;						// TB to TC
	typedef std::map<std::string, inner_promotion_map_t> promotion_map_t;					// TA to map (TB -> TC)
	
	typedef std::function<value_base_sp (property_base*)> promoter_func_t;					// promote value of property to a specified type
	typedef std::map<std::string, promoter_func_t> promoters_inner_map_t;					// target type -> promoter func
	typedef std::map<std::string, promoters_inner_map_t> promoters_map_t;					// source type -> (target type -> promoter func)
	
	typedef std::function<void (property_base*, value_base_sp)> setter_func_t;
	typedef std::map<std::string, setter_func_t> setter_map_t;
	setter_map_t setter_map;
	
	
	promotion_map_t promotion_map;
	promoters_map_t promoters;
};

}
