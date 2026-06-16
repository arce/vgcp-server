#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <FL/Fl_RGB_Image.H>
#include "transform.hpp"

struct Element {
  std::string id, type, grp = "root";
  int parent_idx = -1;
  std::unordered_map<std::string, std::string> attrs;
  Transform transform;
  bool deleted = false;
  bool pos_first = false;
  std::string class_names;
};

struct Symbol {
  std::string id;
  std::string viewbox;
  std::string preserve_aspect;
  std::vector<Element> content;
  std::unordered_map<std::string, int> id_to_idx;
};
