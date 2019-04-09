#include <iostream>
#include <memory>
#include <typeinfo>

#include "lexertl/iterator.hpp"
#include "parsertl/lookup.hpp"
#include "parsertl/generator.hpp"

#include "tree/tree_node.h"
#include "tree/property_listener.h"



tree_node *root = nullptr;
tree_node *interp_root = nullptr;








typedef long long int int_type;
typedef long double real_type;
typedef bool bool_type;















class value_base
{
public:
	virtual ~value_base() {}
	virtual std::string get_type() const = 0;
};

typedef std::shared_ptr<value_base> value_base_sp;
















template <class type>
class value : public value_base
{
	type val;
public:
	value(type v) : val(v) {}
	type get() const { return val; }
	std::string get_type() const
	{
		return typeid(type).name();
	}
};












class term
{
public:
	virtual ~term() {}

	virtual value_base_sp evaluate() const = 0;
};









class const_term : public term
{
public:
	const_term(value_base_sp v) : val(v) {}
	~const_term() {}

	value_base_sp evaluate() const
	{
		return val;
	}
	
private:
	value_base_sp val;
};





class node_term : public term/*, public tree_node, public property_listener*/
{
public:
	node_term(tree_node *root, std::string path)
	{
		this->root = root;
		this->path = path;
	}
	
	~node_term() {}

	value_base_sp evaluate() const
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
};













typedef std::shared_ptr<term> term_sp;

typedef std::stack<term_sp> stack_t;

typedef std::map<std::string, std::string> inner_promotion_map_t;
typedef std::map<std::string, inner_promotion_map_t> promotion_result_t;
promotion_result_t promotion_result;

std::string get_promotion_target(std::string left, std::string right)
{
	promotion_result_t::iterator it = promotion_result.find(left);
	if(it == promotion_result.end())
	{
		return "";
	}
	if(it->second.find(right) == it->second.end())
	{
		return "";
	}
	return promotion_result[left][right];
}

typedef std::map<std::string, std::map<std::string, std::function<value_base_sp(value_base_sp)>>> promoters_t;
promoters_t promoters;

value_base_sp promote(value_base_sp val, std::string type)
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

typedef std::map<std::string, std::function<value_base_sp(value_base_sp, value_base_sp)>> opset_t;
typedef std::map<std::string, opset_t> operations_t;
operations_t operations;

value_base_sp apply(value_base_sp left, value_base_sp right, std::string op)
{
	std::string type = left->get_type();
	if(operations.find(type) == operations.end())
	{
		return nullptr;
	}
	if(operations[type].find(op) == operations[type].end())
	{
		return nullptr;
	}
	return operations[type][op](left, right);
}

value_base_sp do_bin_op(value_base_sp left, value_base_sp right, std::string op)
{
	std::string type_left = left->get_type();
	std::string type_right = right->get_type();

	if(type_left != type_right)
	{
		std::string target_type = get_promotion_target(type_left, type_right);
		left = promote(left, target_type);
		right = promote(right, target_type);
		if(left.get() == nullptr || right.get() == nullptr)
		{
			return nullptr;
		}
	}
	return apply(left, right, op);
}


template <class type>
value_base_sp op(value_base_sp left, value_base_sp right, std::function<type(type, type)> func)
{
	auto left_typed = dynamic_cast<value<type> *>(left.get());
	auto right_typed = dynamic_cast<value<type> *>(right.get());
	if(left_typed == nullptr || right_typed == nullptr)
	{
		return nullptr;
	}
	return value_base_sp(new value<type>(func(left_typed->get(), right_typed->get())));
}

template <class from, class to>
value_base_sp promote(value_base_sp operand, std::function<to(from)> op)
{
	auto typed = dynamic_cast<value<from> *>(operand.get());
	if(typed == nullptr)
	{
		return nullptr;
	}
	return value_base_sp(new value<to>(op(typed->get())));
}

void do_bin_op(std::stack<term_sp> &stack, std::string op)
{
	auto right = stack.top();
	stack.pop();
	auto left = stack.top();
	stack.pop();

	auto right_const = dynamic_cast<const_term *>(right.get());
	auto left_const = dynamic_cast<const_term *>(left.get());
	
	// если оба - константы
	if(left_const != nullptr && right_const != nullptr)
	{
		auto val = do_bin_op(left_const->evaluate(), right_const->evaluate(), op);
		stack.push(term_sp(new const_term(val)));
	}
	else
	{
		//
	}
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
	
	node_term *rec = dynamic_cast<node_term *>(left.get());

	// проверить что слева - узел	
	if(rec == nullptr)
	{
		printf("error: left operand is not of receiver type\n");
		
		return;
	}
	
	// преобразовать терм с вершины стека в отслеживающее выражение???
	
	// 
	
	// добавить отслеживающее выражение в дерево для отслеживающих выражений	
}





int main()
{
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

		std::stack<term_sp> stack;

		operations[typeid(int_type).name()]["mul"] = [](value_base_sp a, value_base_sp b){return op<int_type>(a, b, [](int_type a, int_type b){return a * b;});};
		operations[typeid(int_type).name()]["div"] = [](value_base_sp a, value_base_sp b){return op<int_type>(a, b, [](int_type a, int_type b){return a / b;});};
		operations[typeid(int_type).name()]["sub"] = [](value_base_sp a, value_base_sp b){return op<int_type>(a, b, [](int_type a, int_type b){return a - b;});};
		operations[typeid(int_type).name()]["sum"] = [](value_base_sp a, value_base_sp b){return op<int_type>(a, b, [](int_type a, int_type b){return a + b;});};

		operations[typeid(real_type).name()]["mul"] = [](value_base_sp a, value_base_sp b){return op<real_type>(a, b, [](real_type a, real_type b){return a * b;});};
		operations[typeid(real_type).name()]["div"] = [](value_base_sp a, value_base_sp b){return op<real_type>(a, b, [](real_type a, real_type b){return a / b;});};
		operations[typeid(real_type).name()]["sub"] = [](value_base_sp a, value_base_sp b){return op<real_type>(a, b, [](real_type a, real_type b){return a - b;});};
		operations[typeid(real_type).name()]["sum"] = [](value_base_sp a, value_base_sp b){return op<real_type>(a, b, [](real_type a, real_type b){return a + b;});};

		promotion_result[typeid(real_type).name()][typeid(int_type).name()] = typeid(real_type).name();
		promotion_result[typeid(int_type).name()][typeid(real_type).name()] = typeid(real_type).name();

		promotion_result[typeid(bool_type).name()][typeid(int_type).name()] = typeid(int_type).name();
		promotion_result[typeid(int_type).name()][typeid(bool_type).name()] = typeid(int_type).name();

		promotion_result[typeid(real_type).name()][typeid(bool_type).name()] = typeid(real_type).name();
		promotion_result[typeid(bool_type).name()][typeid(real_type).name()] = typeid(real_type).name();

		promoters[typeid(bool_type).name()][typeid(int_type).name()] = [](value_base_sp op){return promote<bool_type, int_type>(op, [](bool_type val){return ((val == true) ? 1 : 0);});};
		promoters[typeid(int_type).name()][typeid(real_type).name()] = [](value_base_sp op){return promote<int_type, real_type>(op, [](int_type val){return real_type(val);});};
		promoters[typeid(bool_type).name()][typeid(real_type).name()] = [](value_base_sp op){return promote<bool_type, real_type>(op, [](bool_type val){return ((val == true) ? 1.0 : 0.0);});};

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
						stack.push(term_sp(new node_term(interp_root, value_str)));
					}
					else if (rule_ == index_sum)
					{
						do_bin_op(stack, "sum");
					}
					else if (rule_ == index_connect)
					{
						do_connect(stack);
					}
					else if (rule_ == index_sub)
					{
						do_bin_op(stack, "sub");
					}
					else if (rule_ == index_mul)
					{
						do_bin_op(stack, "mul");
					}
					else if (rule_ == index_div)
					{
						do_bin_op(stack, "div");
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
							stack.push(term_sp(new const_term(value_base_sp(new value<bool_type>(val)))));
						}
						else if(value_str.find('.') != std::string::npos)
						{
							char *end = nullptr;
							real_type val = strtod(value_str.c_str(), &end);
							stack.push(term_sp(new const_term(value_base_sp(new value<real_type>(val)))));
						}
						else
						{
							char *end = nullptr;
							int_type val = strtol(value_str.c_str(), &end, 10);
							stack.push(term_sp(new const_term(value_base_sp(new value<int_type>(val)))));
						}
					}

					break;
				}
			}

			parsertl::lookup(parser_state_machine, iter_, results_, productions_);
		}

		if(stack.size() > 0)
		{
			auto val = stack.top()->evaluate();
			auto dval = dynamic_cast<value<real_type> *>(val.get());
			if(dval)
			{
				std::cout << "Result: " << dval->get() << '\n';
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
	tree_node root;
	tree_node *interp_root = root.get("interpreter", true);
		
	
	
	
	
	
	
	std::vector<std::string> statements;
	
	for(int i = 0 ; i < statements.size() ; i += 1)
	{
		evaluate(statements[i]);
	}
	
	return 0;
}
