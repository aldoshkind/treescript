#include <iostream>
#include <memory>
#include <typeinfo>
#include <functional>

#include "lexertl/iterator.hpp"
#include "parsertl/lookup.hpp"
#include "parsertl/generator.hpp"

#include "tree/tree_node.h"
#include "tree/tree_node_inherited.h".h"
#include "tree/property.h"
#include "tree/property_listener.h"



tree_node *root = nullptr;
tree_node *interp_root = nullptr;








typedef long long int int_type;
typedef long double real_type;
typedef bool bool_type;












/*class const_term : public term
{
public:
	const_term(property * v) : val(v) {}
	~const_term() {}

	property * evaluate() const
	{
		return val;
	}
	
private:
	property * val;
};*/





/*class node_term : public term
{
public:
	node_term(tree_node *root, std::string path)
	{
		this->root = root;
		this->path = path;
	}
	
	~node_term() {}

	property * evaluate() const
	{
		return 0;
	}
	
	void subscribe()
	{
		//
	}
private:
	tree_node *root = nullptr;
	std::string path;
};*/

//typedef std::shared_ptr<term> term_sp;



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
	
	virtual void evaluate() = 0;
	virtual void subscribe() = 0;
	
protected:
	op_wrapper_t op;
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
	void evaluate()
	{
		if(this->op)
		{
			op(this);
		}
	}
	
	void updated(property_base */*prop*/)
	{
		evaluate();
	}
};

class tracker : public tree_node, public property_listener
{
public:
	tracker(property_base *tr, property_base *tar) : tracked(tr), target(tar)
	{
		auto op = dynamic_cast<term_op *>(tr);
		if(op != nullptr)
		{
			op->subscribe();
		}		
		
		tracked->add_listener(this);
	}
	
	~tracker()
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



/*template <class value_t>
class prop_term : public tree_node, public property_value<value_t>, term_base
{
public:
	prop_term(tree_node *root, std::string path)
	{
		this->root = root;
		this->path = path;
	}
	
	void evaluate()
	{
		property_base *prop = root->get(path);
		
		if(prop == nullptr)
		{
			return;
		}
		
		property_base::set_value(prop);
	}
	
private:
	tree_node *root = nullptr;
	std::string path;
};*/





typedef std::stack<tree_node *> stack_t;

typedef std::map<std::string, std::string> inner_promotion_map_t;
typedef std::map<std::string, inner_promotion_map_t> promotion_map_t;
promotion_map_t promotion_map;




std::string get_common_type(std::string left, std::string right)
{
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



typedef std::function<property_base* (property_base*)> promoter_func_t;
typedef std::map<std::string, promoter_func_t> promoters_inner_map_t;
typedef std::map<std::string, promoters_inner_map_t> promoters_map_t;
promoters_map_t promoters;

property_base * promote(property_base * val, std::string type)
{
	if(val->get_type() == type)
	{
		return val;
	}
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

typedef std::function<bool (property_base *, property_base *, property_base *)> operation_func_t;
typedef std::map<std::string, operation_func_t> opset_t;
typedef std::map<std::string, opset_t> operations_t;
operations_t operations;


template <class type>
bool op(property_base * left, property_base * right, std::function<type(type, type)> func, property_base *result)
{
	if(result == nullptr)
	{
		return false;
	}
	
	property<type> * left_typed = dynamic_cast<property<type> *>(left);
	property<type> * right_typed = dynamic_cast<property<type> *>(right);
	property<type> * result_typed = dynamic_cast<property<type> *>(result);
	
	property_base *left_promoted = nullptr;
	property_base *right_promoted = nullptr;

	if(left_typed == nullptr)
	{
		left_promoted = left_typed = dynamic_cast<property<type> *>(promote(left, result->property_base::get_type()));
		if(left_typed == nullptr)
		{
			return false;
		}
	}
	
	if(right_typed == nullptr)
	{
		right_promoted = right_typed = dynamic_cast<property<type> *>(promote(right, result->property_base::get_type()));
		if(right_typed == nullptr)
		{
			return false;
		}
	}
	
	result_typed->set_value(func(left_typed->get_value(), right_typed->get_value()));
		
	return true;
}

template <class from, class to>
property_base *promote(property_base *operand, std::function<to(from)> op)
{
	auto typed = dynamic_cast<property<from> *>(operand);
	if(typed == nullptr)
	{
		return nullptr;
	}
	return new property_value<to>(op(typed->get_value()));
}

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
	auto right = stack.top();
	stack.pop();
	auto left = stack.top();
	stack.pop();
	
	auto right_prop = dynamic_cast<property_base *>(right);
	auto left_prop = dynamic_cast<property_base *>(left);

	std::string type = get_common_type(right_prop->property_base::get_type(), left_prop->property_base::get_type());
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
	
	n->set_operation([type, op](property_base *p) -> bool
	{
		tree_node *tn = dynamic_cast<tree_node *>(p);
		if(tn == nullptr)
		{
			return false;
		}
		
		auto left = dynamic_cast<property_base *>(tn->get("left", false));
		auto right = dynamic_cast<property_base *>(tn->get("right", false));
		
		if(left == nullptr || right == nullptr)
		{
			return false;
		}
		
		return operations[type][op](left, right, p);
	});
	
	stack.push(tn);
	
	return true;
}



void do_connect(stack_t &stack)
{
	printf("convert_to_tracker\n");
	
	if(stack.size() < 2)
	{
		printf("error: stack size is less than two\n");
		return;
	}
	
	auto right = stack.top();
	stack.pop();
	auto left = stack.top();
	stack.pop();
	
	tree_node *rec = dynamic_cast<tree_node *>(left);

	// проверить что слева - узел	
	if(rec == nullptr)
	{
		printf("error: left operand is not of receiver type\n");
		
		return;
	}
	
	auto right_prop = dynamic_cast<property_base *>(right);
	auto left_prop = dynamic_cast<property_base *>(left);
	
	if(right_prop == nullptr || left_prop == nullptr)
	{
		return;
	}
	
	tree_node *tr = new tracker(right_prop, left_prop);
	
	interp_root->attach("tracker@" + std::to_string((long long)tr), tr);
	
	stack.push(left);
}


tree_node *generate(std::string type)
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



int main()
{
	tree_node root;
	root.attach("lat", new tree_node_inherited<property_value<real_type>>);
	tree_node *interp_root = root.get("interpreter", true);
	
	::root = &root;
	::interp_root = interp_root;
	
	parsertl::rules parser_rules;
	parsertl::state_machine parser_state_machine;
	lexertl::rules lexer_rules;
	lexertl::state_machine lexer_state_machine;

	try
	{
		parser_rules.token("CONST");
		parser_rules.token("PROP");
		parser_rules.left("'<-'");
		parser_rules.left("'+' '-'");
		parser_rules.left("'*' '/'");
		parser_rules.precedence("UMINUS");

		parser_rules.push("start", "exp");

		const std::size_t index_sum = parser_rules.push("exp", "exp '+' exp");
		const std::size_t index_sub = parser_rules.push("exp", "exp '-' exp");
		const std::size_t index_mul = parser_rules.push("exp", "exp '*' exp");
		const std::size_t index_div = parser_rules.push("exp", "exp '/' exp");
		const std::size_t index_connect = parser_rules.push("exp", "exp '<-' exp");

		parser_rules.push("exp", "'(' exp ')'");

		const std::size_t umin_index_ = parser_rules.push("exp", "'-' exp %prec UMINUS");
		const std::size_t index_const = parser_rules.push("exp", "CONST");
		const std::size_t index_prop = parser_rules.push("exp", "PROP");

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

		std::string expression("lat <- 3 * (70 + 50.0) - true");
		lexertl::citerator iter_(expression.c_str(), expression.c_str() + expression.size(), lexer_state_machine);		
		
		parsertl::match_results results_(iter_->id, parser_state_machine);
		using token = parsertl::token<lexertl::citerator>;
		token::token_vector productions_;

		std::stack<tree_node *> stack;

		operations[typeid(int_type).name()]["mul"] = [](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a * b;}, res);};
		operations[typeid(int_type).name()]["div"] = [](property_base * a, property_base * b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a / b;}, res);};
		operations[typeid(int_type).name()]["sub"] = [](property_base * a, property_base * b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a - b;}, res);};
		operations[typeid(int_type).name()]["sum"] = [](property_base * a, property_base * b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a + b;}, res);};

		operations[typeid(real_type).name()]["mul"] = [](property_base * a, property_base * b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a * b;}, res);};
		operations[typeid(real_type).name()]["div"] = [](property_base * a, property_base * b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a / b;}, res);};
		operations[typeid(real_type).name()]["sub"] = [](property_base * a, property_base * b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a - b;}, res);};
		operations[typeid(real_type).name()]["sum"] = [](property_base * a, property_base * b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a + b;}, res);};

		promotion_map[typeid(real_type).name()][typeid(int_type).name()] = typeid(real_type).name();
		promotion_map[typeid(int_type).name()][typeid(real_type).name()] = typeid(real_type).name();

		promotion_map[typeid(bool_type).name()][typeid(int_type).name()] = typeid(int_type).name();
		promotion_map[typeid(int_type).name()][typeid(bool_type).name()] = typeid(int_type).name();

		promotion_map[typeid(real_type).name()][typeid(bool_type).name()] = typeid(real_type).name();
		promotion_map[typeid(bool_type).name()][typeid(real_type).name()] = typeid(real_type).name();

		promoters[typeid(bool_type).name()][typeid(int_type).name()] = [](property_base * op){return promote<bool_type, int_type>(op, [](bool_type val){return ((val == true) ? 1 : 0);});};
		promoters[typeid(int_type).name()][typeid(real_type).name()] = [](property_base * op){return promote<int_type, real_type>(op, [](int_type val){return real_type(val);});};
		promoters[typeid(bool_type).name()][typeid(real_type).name()] = [](property_base * op){return promote<bool_type, real_type>(op, [](bool_type val){return ((val == true) ? 1.0 : 0.0);});};

		while (results_.entry.action != parsertl::error && results_.entry.action != parsertl::accept)
		{
			switch (results_.entry.action)
			{
				case parsertl::error:
					throw std::runtime_error("Parser error");
					break;
				case parsertl::shift:
				case parsertl::go_to:
				case parsertl::accept:
					break;
				case parsertl::reduce:
				{
					std::size_t rule_ = results_.reduce_id();
					if (rule_ == index_prop)
					{
						printf("var %s\n", results_.dollar(parser_state_machine, 0, productions_).str().c_str());

						std::string value_str = results_.dollar(parser_state_machine, 0, productions_).str();
						//term_sp(new node_term(interp_root, value_str))
						
						tree_node *n = ::root->get(value_str, false);
						if(n == nullptr)
						{
							printf("no such node '%s'\n", value_str.c_str());
							return -1;
						}
						
						stack.push(n);
					}
					else if (rule_ == index_sum)
					{
						push_bin_op(stack, "sum");
					}
					else if (rule_ == index_connect)
					{
						do_connect(stack);
					}
					else if (rule_ == index_sub)
					{
						push_bin_op(stack, "sub");
					}
					else if (rule_ == index_mul)
					{
						push_bin_op(stack, "mul");
					}
					else if (rule_ == index_div)
					{
						push_bin_op(stack, "div");
					}
					else if (rule_ == umin_index_)
					{
						//do_op(stack, "inv");
					}
					else if (rule_ == index_const)
					{
						std::string value_str = results_.dollar(parser_state_machine, 0, productions_).str();
						if(value_str == "true" || value_str == "false")
						{
							bool_type val = (value_str == "true");
							stack.push(new tree_node_inherited<property_value<bool_type>>(val));
						}
						else if(value_str.find('.') != std::string::npos)
						{
							char *end = nullptr;
							real_type val = strtod(value_str.c_str(), &end);
							stack.push(new tree_node_inherited<property_value<real_type>>(val));
						}
						else
						{
							char *end = nullptr;
							int_type val = strtol(value_str.c_str(), &end, 10);
							stack.push(new tree_node_inherited<property_value<int_type>>(val));
						}
					}

					break;
				}
			}

			parsertl::lookup(parser_state_machine, iter_, results_, productions_);
		}

		if(stack.size() > 0)
		{
			auto val = stack.top();
			/*auto tm = dynamic_cast<term_op *>(val);
			if(tm != nullptr)
			{
				tm->evaluate();
			}*/
			auto dval = dynamic_cast<property_value<real_type> *>(val);
			if(dval)
			{
				std::cout << "Result: " << dval->get_value() << '\n';
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cout << e.what() << '\n';
	}

	return 0;
}

void evaluate(std::string statement)
{
	//
}

void setup(tree_node *root, tree_node *interp_root)
{
	::root = root;
	::interp_root = interp_root;
}

int newmain()
{
	std::vector<std::string> statements;
	
	for(int i = 0 ; i < statements.size() ; i += 1)
	{
		evaluate(statements[i]);
	}
	
	return 0;
}
