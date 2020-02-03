#pragma once

#include <functional>

#include <QStringList>
#include <QMap>

#include <tree/tree_node.h>
#include <tree/tree_node_inherited.h>
#include <tree/property.h>

#include "typesystem.h"

namespace treescript
{

typedef std::function<bool (property_base *)> op_wrapper_t;

class wait_node;

class wait_node_listener
{
public:
	virtual ~wait_node_listener(){}
	virtual void node_appeared(wait_node *from, tree_node *which) = 0;
};

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
	
	virtual void set_op_name(std::string op)
	{
		name = op;
	}
	
	void node_appeared()
	{
		evaluate();
	}
	
	virtual void evaluate() = 0;
	virtual void subscribe() = 0;
	
protected:
	op_wrapper_t op;
	std::string name;
};




template <class value_t>
class term : public tree_node, public property_value<value_t>, public term_op, public property_listener, public wait_node_listener
{
public:
	term(value_t v = value_t()) : property_value<value_t>(v)
	{
		tree_node::set_type("term");
	}
	
	value_t					get_value						() const
	{
		return property_value<value_t>::get_value();
	}
	
	void subscribe()
	{
		auto children = get_children();
		for(auto ch : children)
		{
			auto op = dynamic_cast<term_op *>(ch.second);
			auto prop = dynamic_cast<property_base *>(ch.second);
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
	void set_op_name(std::string op)
	{
		tree_node::set_type("term " + op);
		term_op::set_op_name(op);
	}
	
	void update()
	{
		if(this->op)
		{
			op(this);
		}
	}
	
	void node_appeared(wait_node */*from*/, tree_node *which)
	{
		property<value_t> *n = dynamic_cast<property<value_t> *>(which);
		if(n == nullptr)
		{
			return;
		}
		n->add_listener(this);
	}
	
	void updated(property_base */*prop*/)
	{
		evaluate();
	}
	
	void evaluate()
	{
		update();
	}
};






/*class absent_node : public tree_node
{
public:
	absent_node(const std::string &p, tree_node *r) : path(p), root(r)
	{
		//
	}
	
	std::string get_absent_path() const
	{
		return path;
	}
	
	tree_node *get_absent_root() const
	{
		return root;
	}
private:
	std::string path;
	tree_node *root = nullptr;
};*/






class wait_node : public tree_node_inherited<property_value<QString>>, public tree_node::listener_t
{
public:
	wait_node(const std::string &path, tree_node *root)
	{
		tree_node::set_type("wait_node");
		set_value(QString::fromStdString(path));
		this->path = path;
		this->root = root;
	}
	
	void do_wait()
	{
		do_wait(path, root);
	}
	
	void do_wait(const std::string &path, tree_node *root)
	{
		this->root = root;
		set_value(QString::fromStdString(path));
		this->path = path;
		QStringList parts = clean_path(QString::fromStdString(path));
		QString rest = build_path(parts);
//		qDebug() << rest;
		node_to_rest_of_path_map[root] = rest;
		root->add_listener(this);
	}
	
	tree_node *get_node() const
	{
		return node;
	}

private:
	tree_node *root = nullptr;
	tree_node *node = nullptr;
	std::string path;
	typedef QMap<tree_node *, QString> node_to_rest_of_path_map_t;
	node_to_rest_of_path_map_t node_to_rest_of_path_map;
	
	QStringList clean_path(const QString &path)
	{
		QStringList parts = path.split("/", QString::SkipEmptyParts);
		//qDebug() << parts;
		for(int i =	0 ; i < parts.size() ; i += 1)
		{
			if(parts[i] == ".")
			{
				parts.removeAt(i);
				i -= 1;
				continue;
			}
			if(parts[i] == "..")
			{
				parts.removeAt(i);
				if(i > 0)
				{
					parts.removeAt(i - 1);
					i -= 1;
				}
				i -= 1;
			}
		}
		//qDebug() << parts;
		return parts;
	}
	
	QString build_path(QStringList l)
	{
		return QString::fromStdString(l.join("/").toStdString());
	}
	
	void child_added(tree_node *parent, const std::string &, tree_node *n) override
	{
		if(node_to_rest_of_path_map.contains(parent))
		{
			QStringList pts = node_to_rest_of_path_map[parent].split("/", QString::SkipEmptyParts);
			if(pts.size() > 1 && pts[0].toStdString() == name)
			{
				pts.removeAt(0);
				node_to_rest_of_path_map[n] = build_path(pts);
				n->add_listener(this);
			}
			else if(pts.size() == 1 && pts[0].toStdString() == name)
			{
				process_node_appeared(n);
			}
		}
	}
	
	void				child_removed							(tree_node */*parent*/, std::string/* name*/, tree_node */*removed_child*/) override
	{
		//
	}
	
	void process_node_appeared(tree_node *n)
	{
		printf("%s: \"%s\" appeared and is of type \"%s\"\n", __func__, n->get_name().c_str(), n->get_type().c_str());
		node = n;
		
		tree_node *parent = const_cast<tree_node *>(get_owner());
		wait_node_listener *listener = dynamic_cast<wait_node_listener *>(parent);
		if(listener != nullptr)
		{
			listener->node_appeared(this, n);
		}
	}
};


class tracker : public tree_node, public property_listener, public wait_node_listener
{
public:
	tracker(tree_node *tr, property_base *tar, typesystem *ts) : tracked(tr), target(tar)
	{
		subscribe_on_node(tracked);
		this->ts = ts;
		tree_node::set_type("<-");
	}
	
	virtual ~tracker()
	{
		//
	}
	
	void					updated						(property_base *prop)
	{
		if(ts != nullptr)
		{
			ts->convert(prop, target);
		}
		else
		{
			target->set_value(prop);
		}
	}
	
private:
	tree_node *tracked = nullptr;
	property_base *target = nullptr;
	typesystem *ts = nullptr;
	
	void subscribe_on_node(tree_node *n)
	{
		property_base *prop_tracked = dynamic_cast<property_base *>(n);
		if(prop_tracked != nullptr)
		{
			auto op = dynamic_cast<term_op *>(n);
			if(op != nullptr)
			{
				op->subscribe();
			}
			prop_tracked->add_listener(this);
		}
	}
	
	void node_appeared(wait_node */*from*/, tree_node *which)
	{
		subscribe_on_node(which);
	}
};

}
