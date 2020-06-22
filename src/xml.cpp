#include "xml.h"
#include <sstream>

XMLNode::XMLNode(const std::string& name) {
  this->name = name;
}

XMLNode::XMLNode(const std::string& name, const std::vector<std::pair<std::string, std::string>>& attributes) {
  this->name = name;
  this->attributes = attributes;
}

bool XMLNode::empty() const {
  return name.empty();
}

std::string XMLNode::write() const {
  std::ostringstream out;

  if (!name.empty()) {
    out << "<" << name;
    for (const auto& [attr_name, attr_value] : attributes) {
      out << " " << attr_name << "=\"" << attr_value << "\"";
    }

    if (nodes.size() > 0) {
      out << ">";
      for (const auto& node : nodes)
        out << node.write();

      out << "</" << name << ">";
    } else {
      out << "/>";
    }
  }

  return out.str();
}

const std::vector<XMLNode> XMLNodes(const std::vector<XMLNode> &nodes) {
  return nodes;
}

XML::XML() {
  version = "1.0";
  encoding = "UTF-8";
  standalone = "yes";
}

XML::XML(const std::string &version, const std::string &encoding, const std::string &standalone) {
  this->version = version;
  this->encoding = encoding;
  this->standalone = standalone;
}

void XML::setRoot(XMLNode &root) {
  this->root = root;
}

void XML::setRoot(XMLNode &&root) {
  this->root = root;
}

std::string XML::write() {
  std::ostringstream out;

  out << "<?xml version=\"" << version << "\" encoding=\"" << encoding << "\" standalone=\"" << standalone << "\"?>\n";

  if (!root.empty())
    out << root.write();

  return out.str();
}
