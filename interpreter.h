#pragma once

#include <tree/tree_node.h>

namespace treescript
{

class interpreter : public tree_node
{
public:
	interpreter(tree_node *root);
	
	tree_node *eval(std::string expression, tree_node *local_root = nullptr);

	void set_root(tree_node *r);
	
private:
	class internal;
	internal *in = nullptr;
};




}
