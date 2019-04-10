#pragma once

#include <functional>

#include <tree/tree_node.h>
#include <tree/property.h>

namespace treescript
{

typedef std::function<bool (property_base *)> op_wrapper_t;

class term_op
{
public:
	virtual ~term_op()
	{
		//
	}
	
	void set_operation(op_wrapper_t op)
	{
		this->op = op;
	}
	
	void set_op_name(std::string op)
	{
		name = op;
	}
	
	//virtual void evaluate() = 0;
	virtual void subscribe() = 0;
	
protected:
	op_wrapper_t op;
	std::string name;
};




template <class value_t>
class term : public tree_node, public property_value<value_t>, public term_op, public property_listener
{
public:
	value_t					get_value						() const
	{
		return property_value<value_t>::get_value();
	}
	
	void subscribe()
	{
		auto children = get_children();
		for(int i = 0 ; i < children.size() ; i += 1)
		{
			auto op = dynamic_cast<term_op *>(children[i]);
			auto prop = dynamic_cast<property_base *>(children[i]);
			if(op != nullptr)
			{
				op->subscribe();
			}
			if(prop != nullptr)
			{
				prop->add_listener(this);
			}
		}
	}
	
private:
	void update()
	{
		if(this->op)
		{
			op(this);
		}
	}
	
	void updated(property_base *prop)
	{
		update();
	}
};

class tracker : public tree_node, public property_listener
{
public:
	tracker(property_base *tr, property_base *tar) : tracked(tr), target(tar)
	{
		auto op = dynamic_cast<term_op *>(tr);
		tracked->add_listener(this);
		if(op != nullptr)
		{
			op->subscribe();
		}		
	}
	
	virtual ~tracker()
	{
		//
	}
	
	void					updated						(property_base *prop)
	{
		target->set_value(prop);
	}
	
private:
	property_base *tracked = nullptr;
	property_base *target = nullptr;
};

}
