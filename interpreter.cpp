#include "interpreter.h"

using namespace treescript;

tree_node *interpreter::eval(std::string expression)
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
						stack.push(new term<bool_type>(val));
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

void interpreter::do_connect(stack_t &stack)
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

void interpreter::do_assign(stack_t &stack)
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


void interpreter::cleanup_subtree(tree_node *n)
{
	if(dynamic_cast<term_op *>(n) != nullptr)
	{
		delete n;
	}
}
