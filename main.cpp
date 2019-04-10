#include "interpreter.h"

using namespace treescript;

int main()
{
	tree_node root;
	tree_node_inherited<property_value<real_type>> *lat = new tree_node_inherited<property_value<real_type>>;
	tree_node_inherited<property_value<real_type>> *lon = new tree_node_inherited<property_value<real_type>>;
	root.attach("lat", lat);
	root.attach("lon", lon);
	tree_node *interp_root = root.get("interpreter", true);
	
	
	
	interpreter interp(&root, interp_root);
	
	
	
	std::vector<std::string> statements;
	
	statements.push_back("lat <- 3 * (70 + 50.0) - lon");
	
	for(std::vector<std::string>::size_type i = 0 ; i < statements.size() ; i += 1)
	{
		interp.eval(statements[i]);
		printf("%f\n", lat->get_value());
		lon->set_value(10);
		interp.eval(statements[i]);
		printf("%f\n", lat->get_value());
	}

	return 0;
}
