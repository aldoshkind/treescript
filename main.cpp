#include "interpreter.h"

#include <treecmd/cmd.h>

using namespace treescript;

class interp : public treecmd::interpreter, public treescript::interpreter
{
public:
	interp(tree_node *root, tree_node *interpreter_root) : treescript::interpreter(root, interpreter_root)
	{
		//
	}

	tree_node *evaluate(std::string s)
	{
		return eval(s);
	}

	void set_root(tree_node *root)
	{
		treescript::interpreter::set_root(root);
	}
};

int main()
{
	tree_node root;
	tree_node_inherited<property_value<real_type>> *lat = new tree_node_inherited<property_value<real_type>>;
	tree_node_inherited<property_value<real_type>> *lon = new tree_node_inherited<property_value<real_type>>;
	root.attach("lat", lat);
	root.attach("lon", lon);
	tree_node *interp_root = root.get("interpreter", true);
	
	treecmd::cmd cmd(&root);
	
	interp interp(&root, interp_root);
	
	cmd.set_interpreter(&interp);
	cmd.run_in_thread();

	for( ; ; )
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}
