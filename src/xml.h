#pragma once
#include <vector>
#include <string>

class XMLNode;

class XMLNode {
  std::string name;
  std::string text;
  std::vector<std::pair<std::string, std::string>> attributes;
  std::vector<XMLNode> nodes;

public:
  XMLNode() = default;
  XMLNode(const std::string& name);
  XMLNode(const std::string& name, const std::string& text);
  XMLNode(const std::string& name, const std::vector<std::pair<std::string, std::string>>& attributes);
  XMLNode(const std::string& name, const std::vector<std::pair<std::string, std::string>>& attributes, const std::string& text);
  std::string write() const;
  bool empty() const;
  static std::string XMLText(const std::string& str);

  friend XMLNode& operator<<(XMLNode&& parent, const XMLNode& child) {
    parent.nodes.push_back(child);
    return parent;
  }

  friend XMLNode& operator<<(XMLNode& parent, const XMLNode& child) {
    parent.nodes.push_back(child);
    return parent;
  }

  friend XMLNode& operator<<(XMLNode&& parent, const std::vector<XMLNode>& children) {
    for (const auto &child : children)
      parent.nodes.push_back(child);

    return parent;
  }

  friend XMLNode& operator<<(XMLNode& parent, const std::vector<XMLNode>& children) {
    for (const auto &child : children)
      parent.nodes.push_back(child);

    return parent;
  }

};

const std::vector<XMLNode> XMLNodes(const std::vector<XMLNode> &nodes);

class XML {
  std::string version;
  std::string encoding;
  std::string standalone;
  XMLNode root;

public:
  XML();
  XML(const std::string &version, const std::string &encoding, const std::string &standalone);
  void setRoot(XMLNode &root);
  void setRoot(XMLNode &&root);

  std::string write();
};


