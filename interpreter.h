#pragma once

#include "lexertl/iterator.hpp"
#include "parsertl/lookup.hpp"
#include "parsertl/generator.hpp"

#include <tree/property.h>
#include <tree/tree_node.h>
#include <tree/tree_node_inherited.h>

#include "term.h"
#include "typesystem.h"

namespace treescript
{



class interpreter
{
public:
	interpreter(tree_node *root, tree_node *interpreter_root)
	{
		this->root = root;
		this->interp_root = interpreter_root;
		
		init();
		init_types();
	}
	
	void init()
	{
		try
		{
			parser_rules.token("CONST");
			parser_rules.token("PROP");
			parser_rules.left("'<-'");
			parser_rules.left("'+' '-'");
			parser_rules.left("'*' '/'");
			parser_rules.precedence("UMINUS");
	
			parser_rules.push("start", "exp");
	
			index_sum = parser_rules.push("exp", "exp '+' exp");
			index_sub = parser_rules.push("exp", "exp '-' exp");
			index_mul = parser_rules.push("exp", "exp '*' exp");
			index_div = parser_rules.push("exp", "exp '/' exp");
			index_connect = parser_rules.push("exp", "exp '<-' exp");
	
			parser_rules.push("exp", "'(' exp ')'");
	
			umin_index_ = parser_rules.push("exp", "'-' exp %prec UMINUS");
			index_const = parser_rules.push("exp", "CONST");
			index_prop = parser_rules.push("exp", "PROP");
	
			parsertl::generator::build(parser_rules, parser_state_machine);
	
			lexer_rules.push("[+]", parser_rules.token_id("'+'"));
			lexer_rules.push("-", parser_rules.token_id("'-'"));
			lexer_rules.push("<-", parser_rules.token_id("'<-'"));
			lexer_rules.push("[*]", parser_rules.token_id("'*'"));
			lexer_rules.push("[/]", parser_rules.token_id("'/'"));
			lexer_rules.push("(\\d+([.]\\d+)?)|(true|false)", parser_rules.token_id("CONST"));
			lexer_rules.push("(([/]*(\\w+|\\.{1,2})[/]*)+)", parser_rules.token_id("PROP"));
			//lexer_rules.push("[a-zA-Z]+", parser_rules.token_id("PROP"));
			lexer_rules.push("[(]", parser_rules.token_id("'('"));
			lexer_rules.push("[)]", parser_rules.token_id("')'"));
			lexer_rules.push("\\s+", lexer_rules.skip());
	
			lexertl::generator::build(lexer_rules, lexer_state_machine);
		}
		catch (const std::exception &e)
		{
			std::cout << e.what() << '\n';
		}
	}
	
	void init_types()
	{
		operations[typeid(int_type).name()]["mul"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){printf("*");return a * b;}, res);};
		operations[typeid(int_type).name()]["div"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){printf("/");return a / b;}, res);};
		operations[typeid(int_type).name()]["sub"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){printf("-");return a - b;}, res);};
		operations[typeid(int_type).name()]["sum"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){printf("+");return a + b;}, res);};

		operations[typeid(real_type).name()]["mul"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){printf("*");return a * b;}, res);};
		operations[typeid(real_type).name()]["div"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){printf("/");return a / b;}, res);};
		operations[typeid(real_type).name()]["sub"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){printf("-");return a - b;}, res);};
		operations[typeid(real_type).name()]["sum"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){printf("+");return a + b;}, res);};
	}
	
	bool eval(std::string expression);
	
private:
	tree_node *root = nullptr;
	tree_node *interp_root = nullptr;
	
	typedef std::stack<tree_node *> stack_t;
	
	parsertl::rules parser_rules;
	parsertl::state_machine parser_state_machine;
	lexertl::rules lexer_rules;
	lexertl::state_machine lexer_state_machine;
	
	
	
	typedef std::function<bool (property_base *, property_base *, property_base *)> operation_func_t;
	typedef std::map<std::string, operation_func_t> opset_t;
	typedef std::map<std::string, opset_t> operations_t;
	operations_t operations;
	
	
	typesystem ts;
	
	std::size_t index_sum;
	std::size_t index_sub;
	std::size_t index_mul;
	std::size_t index_div;
	std::size_t index_connect;

	std::size_t umin_index_;
	std::size_t index_const;
	std::size_t index_prop;

	
	term_op *generate_op_node(std::string type)
	{
		if(type == typeid(bool_type).name())
		{
			return new term<bool_type>;
		}
		if(type == typeid(real_type).name())
		{
			return new term<real_type>;
		}
		if(type == typeid(int_type).name())
		{
			return new term<int_type>;
		}
		
		return nullptr;
	}
	
	bool push_bin_op(stack_t &stack, std::string op)
	{
		if(stack.size() < 2)
		{
			return false;
		}
		
		auto right = stack.top();
		stack.pop();
		auto left = stack.top();
		stack.pop();
		
		auto right_prop = dynamic_cast<property_base *>(right);
		auto left_prop = dynamic_cast<property_base *>(left);
	
		std::string type = ts.get_common_type(right_prop->property_base::get_type(), left_prop->property_base::get_type());
		if(type == "")
		{
			return false;
		}
		term_op *n = generate_op_node(type);
		tree_node *tn = dynamic_cast<tree_node *>(n);
		if(n == nullptr || tn == nullptr)
		{
			return false;
		}
		tn->attach("left", left);
		tn->attach("right", right);
		
		
		if(operations.find(type) == operations.end())
		{
			return false;
		}
		if(operations[type].find(op) == operations[type].end())
		{
			return false;
		}	
		
		n->set_operation([type, op, this](property_base *p) -> bool
		{
			tree_node *tn = dynamic_cast<tree_node *>(p);
			if(tn == nullptr)
			{
				return false;
			}
			
			if(tn->get_children().size() < 2)
			{
				return false;
			}
			
			auto left = dynamic_cast<property_base *>(tn->get_children()[0]);
			auto right = dynamic_cast<property_base *>(tn->get_children()[1]);
			
			if(left == nullptr || right == nullptr)
			{
				return false;
			}
			
			return operations[type][op](left, right, p);
		});
		
		n->set_op_name(op);
		stack.push(tn);
		
		return true;
	}
	
	
	
	void do_connect(stack_t &stack);
	
	
	template <class type>
	bool op(property_base *left, property_base *right, std::function<type(type, type)> func, property_base *result)
	{
		if(result == nullptr)
		{
			return false;
		}
		
		property<type> *left_typed = dynamic_cast<property<type> *>(left);
		property<type> *right_typed = dynamic_cast<property<type> *>(right);
		property<type> *result_typed = dynamic_cast<property<type> *>(result);
		
		type left_value = type();
		type right_value = type();
	
		if(left_typed == nullptr)
		{
			value_base_sp left_value_promoted = ts.promote_to(left, result->property_base::get_type());
			if(left_value_promoted.get() == nullptr)
			{
				return false;
			}
			left_value = dynamic_cast<value<type> *>(left_value_promoted.get())->get_value();
		}
		else
		{
			left_value = left_typed->get_value();
		}
		
		
		if(right_typed == nullptr)
		{
			value_base_sp right_value_promoted = ts.promote_to(right, result->property_base::get_type());
			if(right_value_promoted.get() == nullptr)
			{
				return false;
			}
			right_value = dynamic_cast<value<type> *>(right_value_promoted.get())->get_value();
		}
		else
		{
			right_value = right_typed->get_value();
		}
		
		type val = func(left_value, right_value);
		result_typed->set_value(val);
			
		return true;
	}
	
};

}
