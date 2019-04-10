#pragma once

#include <memory>

namespace treescript
{

class value_base
{
public:
	virtual ~value_base() {}
};

template <class value_type>
class value : public value_base
{
public:
	value(value_type v = value_type()) : val(v)
	{
		//
	}
	
	value_type get_value()
	{
		return val;
	}
	
private:
	value_type val;
};

typedef std::shared_ptr<value_base> value_base_sp;

}
