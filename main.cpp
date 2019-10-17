#include "interpreter.h"

#include <QDebug>

#include <treecmd/cmd.h>
#include "types.h"

using namespace treescript;





class interp : public treecmd::interpreter, public treescript::interpreter
{
public:
	interp(tree_node *root) : treescript::interpreter(root)
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
	interp in(&root);
	root.attach("interpreter", &in, false);

	treecmd::cmd cmd(&root);
	
	cmd.set_interpreter(&in);
	cmd.run_in_thread();

	for( ; ; )
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}
