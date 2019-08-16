#include "interpreter.h"

#include <functional>

#include "lexertl/lexertl/iterator.hpp"
#include "parsertl/parsertl/lookup.hpp"
#include "parsertl/parsertl/generator.hpp"

#include <tree/property.h>
#include <tree/tree_node_inherited.h>

#include "term.h"
#include "typesystem.h"

using namespace treescript;

class interpreter::internal
{
public:
	internal(tree_node *root, tree_node *interpreter_root);
	
	tree_node *eval(std::string expression);
	
	void set_root(tree_node *r)
	{
		root = r;
	}
	
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
	std::size_t index_assign;

	//std::size_t umin_index_;
	std::size_t index_const;
	std::size_t index_prop;
	std::size_t index_string;

	void init();
	void init_types();
	
	term_op *generate_op_node(std::string type);	
	bool push_bin_op(stack_t &stack, std::string op);	
	void do_connect(stack_t &stack);
	void do_assign(stack_t &stack);	
	void cleanup_subtree(tree_node *n);
	
	template <class type>
	bool op(property_base *left, property_base *right, std::function<type(type, type)> func, property_base *result);
	
};

template <class optype>
bool interpreter::internal::op(property_base *left, property_base *right, std::function<optype(optype, optype)> func, property_base *result)
{
	if(result == nullptr)
	{
		return false;
	}
	
	property<optype> *left_typed = dynamic_cast<property<optype> *>(left);
	property<optype> *right_typed = dynamic_cast<property<optype> *>(right);
	property<optype> *result_typed = dynamic_cast<property<optype> *>(result);
	
	optype left_value = optype();
	optype right_value = optype();

	if(left_typed == nullptr)
	{
		value_base_sp left_value_promoted = ts.promote_to(left, result->property_base::get_type());
		if(left_value_promoted.get() == nullptr)
		{
			return false;
		}
		left_value = dynamic_cast<value<optype> *>(left_value_promoted.get())->get_value();
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
		right_value = dynamic_cast<value<optype> *>(right_value_promoted.get())->get_value();
	}
	else
	{
		right_value = right_typed->get_value();
	}
	
	optype val = func(left_value, right_value);
	result_typed->set_value(val);
		
	return true;
}

interpreter::interpreter(tree_node *root)
{
	in = new internal(root, this);
}

void interpreter::set_root(tree_node *r)
{
	in->set_root(r);
}

tree_node *interpreter::eval(std::string expression)
{
	if(in == nullptr)
	{
		return nullptr;
	}
	return in->eval(expression);
}


interpreter::internal::internal(tree_node *root, tree_node *interpreter_root)
{
	this->root = root;
	this->interp_root = interpreter_root;
	
	init();
	init_types();
}

tree_node *interpreter::internal::eval(std::string expression)
{
	lexertl::citerator iter_(expression.c_str(), expression.c_str() + expression.size(), lexer_state_machine);		
	
	parsertl::match_results results_(iter_->id, parser_state_machine);
	using token = parsertl::token<lexertl::citerator>;
	token::token_vector productions_;

	std::stack<tree_node *> stack;
	
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
					std::string prop_name = results_.dollar(parser_state_machine, 0, productions_).str();						
					tree_node *n = root->get(prop_name, false);
					if(n == nullptr)
					{
						printf("no such node '%s'\n", prop_name.c_str());
						return nullptr;
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
				else if (rule_ == index_assign)
				{
					do_assign(stack);
				}
				/*else if (rule_ == umin_index_)
				{
					//do_op(stack, "inv");
				}*/
				else if (rule_ == index_const)
				{
					std::string value_str = results_.dollar(parser_state_machine, 0, productions_).str();
					if(value_str == "true" || value_str == "false")
					{
						bool_type val = (value_str == "true");
						stack.push(new term<bool_type>(val));
					}
					else if (value_str[0] == '\'' && value_str[value_str.size() - 1] == '\'')
					{
						string_type val = string_type::fromStdString(value_str.substr(1, value_str.size() - 2));
						stack.push(new term<string_type>(val));
					}
					else if(value_str.find('.') != std::string::npos)
					{
						char *end = nullptr;
						real_type val = strtod(value_str.c_str(), &end);
						stack.push(new term<real_type>(val));
					}
					else
					{
						char *end = nullptr;
						int_type val = strtol(value_str.c_str(), &end, 10);
						stack.push(new term<int_type>(val));
					}
					stack.top()->set_name(value_str);
				}

				break;
			}
		}

		parsertl::lookup(parser_state_machine, iter_, results_, productions_);
	}
	
	if(stack.size() == 1)
	{
		tree_node *n = stack.top();
		
		// If node is a term - we've created it and have to control it's lifetime.
		// Otherwise it's value node from tree and we are not responsible for it.
		// Other term nodes are accessible via tracker@xxxxxx nodes.
		if(dynamic_cast<term_op *>(n) != nullptr)
		{
			// special case when result of operation is same node as result from prev op
			tree_node *old_res = interp_root->get("result", false);
			if(old_res != n)
			{
				interp_root->remove("result", true);
				interp_root->attach("result", n);
			}
		}
		
		return n;
	}
	
#warning
	// cleanup stack!!!

	return nullptr;
}

void interpreter::internal::do_connect(stack_t &stack)
{
	if(stack.size() < 2)
	{
		printf("error: stack size is less than two\n");
		return;
	}
	
	auto right = stack.top();
	stack.pop();
	auto left = stack.top();
	stack.pop();
	
	auto right_prop = dynamic_cast<property_base *>(right);
	auto left_prop = dynamic_cast<property_base *>(left);
	
	if(right_prop == nullptr || left_prop == nullptr)
	{
		return;
	}
	
	typesystem *ts = nullptr;
	if(left_prop->get_type() != right_prop->get_type())
	{
		if(!this->ts.can_convert(right_prop->get_type(), left_prop->get_type()) || !this->ts.can_set(left_prop->get_type()))
		{
			printf("error: cannot connect node of type '%s' to '%s'\n", right_prop->get_type().c_str(), left_prop->get_type().c_str());
			return;
		}
		ts = &this->ts;
	}
	
	tracker *track = new tracker(right_prop, left_prop, ts);
	tree_node *tr = track;
	
	interp_root->attach("tracker@" + std::to_string((long long)tr), tr);
	
	stack.push(left);
}

void interpreter::internal::do_assign(stack_t &stack)
{
	if(stack.size() < 2)
	{
		printf("error: stack size is less than two\n");
		return;
	}
	
	auto right = stack.top();
	stack.pop();
	auto left = stack.top();
	stack.pop();
	
	auto right_prop = dynamic_cast<property_base *>(right);
	auto left_prop = dynamic_cast<property_base *>(left);
	
	if(right_prop == nullptr || left_prop == nullptr)
	{
		printf("error: one or both of operands are not properties\n");
		cleanup_subtree(left);
		cleanup_subtree(right);
		return;
	}
	
	if(dynamic_cast<term_op *>(left_prop) != nullptr)
	{
		printf("error: left operand of assignment is not assignable\n");
		cleanup_subtree(left);
		cleanup_subtree(right);
		return;
	}
	
	if(left_prop->get_type() != right_prop->get_type())
	{
		if(!this->ts.can_convert(right_prop->get_type(), left_prop->get_type()) || !this->ts.can_set(left_prop->get_type()))
		{
			printf("error: cannot connect node of type '%s' to '%s'\n", right_prop->get_type().c_str(), left_prop->get_type().c_str());
			cleanup_subtree(left);
			cleanup_subtree(right);
			return;
		}
		else
		{
			ts.convert(right_prop, left_prop);
		}
	}
	else
	{
		left_prop->set_value(right_prop);
	}
	
	// do not do cleanup left due to left is not term (check is above)
	cleanup_subtree(right);
	
	stack.push(left);
}


void interpreter::internal::cleanup_subtree(tree_node *n)
{
	if(dynamic_cast<term_op *>(n) != nullptr)
	{
		delete n;
	}
}

bool interpreter::internal::push_bin_op(stack_t &stack, std::string op)
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
	tn->attach("left$" + left->get_name(), left);
	tn->attach("right$" + right->get_name(), right);
	
	
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
	
	// Set name. Name will be replaced if node is attached under other node.
	tn->set_name(op);
	n->set_op_name(op);
	n->evaluate();
	stack.push(tn);
	
	return true;
}

term_op *interpreter::internal::generate_op_node(std::string type)
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

void interpreter::internal::init()
{
	try
	{
		parser_rules.token("CONST");
		parser_rules.token("PROP");
		parser_rules.left("'='");
		parser_rules.left("'<-'");
		parser_rules.left("'+' '-'");
		parser_rules.left("'*' '/'");
		//parser_rules.precedence("UMINUS");

		parser_rules.push("start", "exp");

		index_sum = parser_rules.push("exp", "exp '+' exp");
		index_sub = parser_rules.push("exp", "exp '-' exp");
		index_mul = parser_rules.push("exp", "exp '*' exp");
		index_div = parser_rules.push("exp", "exp '/' exp");
		index_connect = parser_rules.push("exp", "exp '<-' exp");
		index_assign = parser_rules.push("exp", "exp '=' exp");

		parser_rules.push("exp", "'(' exp ')'");

		//umin_index_ = parser_rules.push("exp", "'-' exp %prec UMINUS");
		index_const = parser_rules.push("exp", "CONST");
		index_prop = parser_rules.push("exp", "PROP");

		parsertl::generator::build(parser_rules, parser_state_machine);

		lexer_rules.push("[+]", parser_rules.token_id("'+'"));
		lexer_rules.push("-", parser_rules.token_id("'-'"));
		lexer_rules.push("<-", parser_rules.token_id("'<-'"));
		lexer_rules.push("[*]", parser_rules.token_id("'*'"));
		lexer_rules.push("=", parser_rules.token_id("'='"));
		lexer_rules.push("[/]", parser_rules.token_id("'/'"));
		lexer_rules.push("(\\d+([.]\\d+)?)|(true|false)|'.*?'", parser_rules.token_id("CONST"));
		lexer_rules.push("(([/]*(\\w+|\\.{1,2})[/]*)+)", parser_rules.token_id("PROP"));
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

void interpreter::internal::init_types()
{
	operations[typeid(int_type).name()]["mul"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a * b;}, res);};
	operations[typeid(int_type).name()]["div"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a / b;}, res);};
	operations[typeid(int_type).name()]["sub"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a - b;}, res);};
	operations[typeid(int_type).name()]["sum"] = [this](property_base *a, property_base *b, property_base *res){return op<int_type>(a, b, [](int_type a, int_type b){return a + b;}, res);};

	operations[typeid(real_type).name()]["mul"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a * b;}, res);};
	operations[typeid(real_type).name()]["div"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a / b;}, res);};
	operations[typeid(real_type).name()]["sub"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a - b;}, res);};
	operations[typeid(real_type).name()]["sum"] = [this](property_base *a, property_base *b, property_base *res){return op<real_type>(a, b, [](real_type a, real_type b){return a + b;}, res);};
}
