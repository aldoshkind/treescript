#pragma once

#include <tree/tree_node.h>

namespace treescript
{

class interpreter : public tree_node
{
public:
	interpreter(tree_node *root);
	
	tree_node *eval(std::string expression);
	
private:
	class internal;
	internal *in = nullptr;
};




}
