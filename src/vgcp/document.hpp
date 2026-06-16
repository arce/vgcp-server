#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include "types.hpp"
#include "utils.hpp"
#include "color.hpp"

class VGCPDocument {
public:
  std::vector<Element> elements;
  std::unordered_map<std::string, int> id_to_idx;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> styles;
  std::unordered_map<int, std::vector<int>> children;
  std::unordered_map<std::string, Symbol> symbols;
  std::unordered_map<std::string, Fl_RGB_Image*> image_cache;
  int viewport_width = 800;
  int viewport_height = 600;
  std::string bg_color = "#000000";
  bool dirty = true;
  std::string current_symbol_id;
  bool in_symbol_mode = false;

  ~VGCPDocument() {
    for (auto& kv : image_cache) delete kv.second;
  }

  std::string_view parse_value(std::string_view sv, size_t start, size_t& end) {
    while (start < sv.size() && sv[start] == ' ') ++start;
    if (start >= sv.size()) { end = start; return ""; }
    if (sv[start] == '"') {
      ++start;
      size_t close = sv.find('"', start);
      if (close == std::string_view::npos) { end = sv.size(); return sv.substr(start); }
      end = close + 1;
      while (end < sv.size() && sv[end] == ' ') ++end;
      return sv.substr(start, close - start);
    }
    end = sv.find(' ', start);
    if (end == std::string_view::npos) end = sv.size();
    auto val = sv.substr(start, end - start);
    while (end < sv.size() && sv[end] == ' ') ++end;
    return val;
  }

  std::unordered_map<std::string, std::string> parse_params(std::string_view sv, size_t start) {
    std::unordered_map<std::string, std::string> params;
    while (start < sv.size()) {
      while (start < sv.size() && sv[start] == ' ') ++start;
      if (start >= sv.size()) break;
      size_t eq = sv.find('=', start);
      if (eq == std::string_view::npos) break;
      std::string key(sv.substr(start, eq - start));
      size_t val_end;
      std::string val(parse_value(sv, eq + 1, val_end));
      start = val_end;
      params[key] = val;
    }
    return params;
  }

  std::string process_command(const std::string& line) {
    if (line.empty() || line[0] == '#') return "";
    std::string_view sv(line);
    size_t sp = sv.find(' ');
    std::string_view cmd = (sp == std::string_view::npos) ? sv : sv.substr(0, sp);
    size_t after = (sp == std::string_view::npos) ? sv.size() : sp + 1;
    auto params = parse_params(sv, after);

    if (cmd == "SYM") return define_symbol(params);

    if (in_symbol_mode) {
      if (cmd == "RCT" || cmd == "CIR" || cmd == "ELL" || cmd == "LIN" ||
          cmd == "PAT" || cmd == "PGN" || cmd == "PLN" || cmd == "TXT" ||
          cmd == "IMG" || cmd == "GRP") {
        return add_to_symbol(std::string(cmd), params);
      }
    }

    if (cmd == "VIEW") {
      if (!params.count("w") || !params.count("h")) return "ERROR SYNTAX: Missing w or h\n";
      viewport_width  = parse_int(params["w"], 800);
      viewport_height = parse_int(params["h"], 600);
      if (params.count("bg")) bg_color = params["bg"];
      dirty = true;
      return "OK\n";
    }
    if (cmd == "CLR") {
      for (auto& kv : image_cache) delete kv.second;
      image_cache.clear();
      elements.clear();
      id_to_idx.clear();
      children.clear();
      dirty = true;
      return "OK\n";
    }
    if (cmd == "RST") {
      for (auto& kv : image_cache) delete kv.second;
      image_cache.clear();
      elements.clear();
      id_to_idx.clear();
      children.clear();
      styles.clear();
      symbols.clear();
      in_symbol_mode = false;
      current_symbol_id.clear();
      viewport_width  = 800;
      viewport_height = 600;
      bg_color = "#000000";
      dirty = true;
      return "OK\n";
    }
    if (cmd == "BYE") return "BYE\n";

    if (cmd == "RCT" || cmd == "CIR" || cmd == "ELL" || cmd == "LIN" ||
        cmd == "PAT" || cmd == "PGN" || cmd == "PLN" || cmd == "TXT" ||
        cmd == "IMG" || cmd == "GRP") {
      return create_element(std::string(cmd), params);
    }

    if (cmd == "USE")     return use_symbol(params);
    if (cmd == "SYM-DEL") return delete_symbol(params);
    if (cmd == "ATTR")    return update_attributes(params);
    if (cmd == "DEL")     return delete_elements(params);
    if (cmd == "GET")     return get_elements(params);
    if (cmd == "EX")      return exists_element(params);
    if (cmd == "STY")     return set_style(params);
    if (cmd == "STY-DEL") return delete_styles(params);
    if (cmd == "STY-GET") return get_styles(params);

    return "ERROR SYNTAX: Unknown command '" + std::string(cmd) + "'\n";
  }

  void rebuild_hierarchy();
  std::unordered_map<std::string, std::string> resolve_style(
      const Element& elem,
      const std::unordered_map<std::string, std::string>& inherited);
  void draw_symbol_inst(const std::string& sym_id,
                        const std::unordered_map<std::string, std::string>& overrides,
                        const Transform& parent_tr,
                        double offset_x, double offset_y,
                        double override_w, double override_h);
  void draw_element(const Element& elem,
                    const std::unordered_map<std::string, std::string>& inherited,
                    const Transform& parent_tr);
  void draw();
  std::string find_element_at(int px, int py);

private:
  std::string add_to_symbol(const std::string& type,
                             const std::unordered_map<std::string, std::string>& params) {
    auto sym_it = symbols.find(current_symbol_id);
    if (sym_it == symbols.end()) return "ERROR INTERNAL: Active symbol not found\n";
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    const std::string& id_str = params.at("id");
    Symbol& sym = sym_it->second;
    if (sym.id_to_idx.count(id_str))
      return "ERROR DUPLICATE: Element id '" + id_str + "' already exists in symbol\n";
    Element elem;
    elem.id   = id_str;
    elem.type = type;
    elem.grp  = params.count("grp") ? params.at("grp") : "root";
    elem.pos_first   = (params.count("pos") && params.at("pos") == "f");
    elem.class_names = params.count("class") ? params.at("class") : "";
    if (params.count("tr")) elem.transform = parse_transform(params.at("tr"));
    for (auto& kv : params) {
      if (kv.first != "id" && kv.first != "grp" && kv.first != "pos" &&
          kv.first != "class" && kv.first != "tr")
        elem.attrs[kv.first] = kv.second;
    }
    int new_idx = (int)sym.content.size();
    sym.id_to_idx[id_str] = new_idx;
    sym.content.push_back(elem);
    return "OK\n";
  }

  std::string create_element(const std::string& type,
                              const std::unordered_map<std::string, std::string>& params) {
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    const std::string& id_str = params.at("id");
    if (id_to_idx.count(id_str))
      return "ERROR DUPLICATE: Element id '" + id_str + "' already exists\n";
    Element elem;
    elem.id   = id_str;
    elem.type = type;
    elem.grp  = params.count("grp") ? params.at("grp") : "root";
    elem.pos_first   = (params.count("pos") && params.at("pos") == "f");
    elem.class_names = params.count("class") ? params.at("class") : "";
    if (params.count("tr")) elem.transform = parse_transform(params.at("tr"));
    for (auto& kv : params) {
      if (kv.first != "id" && kv.first != "grp" && kv.first != "pos" &&
          kv.first != "class" && kv.first != "tr")
        elem.attrs[kv.first] = kv.second;
    }
    if (type == "IMG" && elem.attrs.count("href")) {
      std::string err = load_image(id_str, elem.attrs["href"]);
      if (!err.empty()) return err;
    }
    int parent_idx = -1;
    if (elem.grp != "root") {
      auto it = id_to_idx.find(elem.grp);
      if (it != id_to_idx.end() && !elements[it->second].deleted)
        parent_idx = it->second;
    }
    elem.parent_idx = parent_idx;
    int new_idx = (int)elements.size();
    elements.push_back(elem);
    id_to_idx[id_str] = new_idx;
    dirty = true;
    return "OK\n";
  }

  std::string load_image(const std::string& id_str, const std::string& href) {
    const std::string prefix = "data:image/";
    if (href.substr(0, prefix.size()) != prefix)
      return "ERROR PARAM: Invalid href format (expected data URI)\n";
    size_t semi = href.find(';', prefix.size());
    if (semi == std::string::npos)
      return "ERROR PARAM: Invalid href format (missing semicolon)\n";
    std::string fmt = href.substr(prefix.size(), semi - prefix.size());
    if (fmt == "svg+xml")
      return "ERROR PARAM: svg+xml image format is not supported\n";
    const std::vector<std::string> allowed = {"png", "jpeg", "jpg", "gif", "webp"};
    if (std::find(allowed.begin(), allowed.end(), fmt) == allowed.end())
      return "ERROR PARAM: Unsupported image format '" + fmt + "'\n";
    const std::string b64_prefix = ";base64,";
    size_t data_start = href.find(b64_prefix, semi);
    if (data_start == std::string::npos)
      return "ERROR PARAM: Invalid href format (missing base64 marker)\n";
    data_start += b64_prefix.size();
    std::string b64 = href.substr(data_start);
    if (b64.size() > 14000000)
      return "ERROR PARAM: Image data exceeds size limit\n";
    auto raw = base64_decode(b64);
    if (raw.empty())
      return "ERROR PARAM: Invalid Base64 data in href\n";
    Fl_RGB_Image* img = nullptr;
    if (fmt == "png") {
      Fl_PNG_Image p(nullptr, raw.data(), (int)raw.size());
      if (!p.fail()) img = (Fl_RGB_Image*)p.copy();
    } else if (fmt == "jpeg" || fmt == "jpg") {
      Fl_JPEG_Image j(nullptr, raw.data());
      if (!j.fail()) img = (Fl_RGB_Image*)j.copy();
    }
    if (!img)
      return "ERROR PARAM: Could not decode image (corrupt or unsupported data)\n";
    auto it = image_cache.find(id_str);
    if (it != image_cache.end()) delete it->second;
    image_cache[id_str] = img;
    return "";
  }

  std::string define_symbol(const std::unordered_map<std::string, std::string>& params) {
    if (params.count("id") && params.at("id") == "end") {
      in_symbol_mode = false;
      current_symbol_id.clear();
      return "OK\n";
    }
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    if (!params.count("vb"))  return "ERROR SYNTAX: Missing vb parameter\n";
    const std::string& sym_id = params.at("id");
    Symbol sym;
    sym.id              = sym_id;
    sym.viewbox         = params.at("vb");
    sym.preserve_aspect = params.count("pr") ? params.at("pr") : "meet";
    symbols[sym_id]     = sym;
    in_symbol_mode      = true;
    current_symbol_id   = sym_id;
    return "OK\n";
  }

  std::string delete_symbol(const std::unordered_map<std::string, std::string>& params) {
    if (params.count("all") && params.at("all") == "t") {
      symbols.clear();
      return "OK\n";
    }
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    if (!symbols.count(params.at("id")))
      return "ERROR NOT_FOUND: Symbol not found\n";
    symbols.erase(params.at("id"));
    return "OK\n";
  }

  std::string use_symbol(const std::unordered_map<std::string, std::string>& params) {
    if (!params.count("id"))  return "ERROR SYNTAX: Missing id parameter\n";
    if (!params.count("sym")) return "ERROR SYNTAX: Missing sym parameter\n";
    const std::string& id_str = params.at("id");
    const std::string& sym_id = params.at("sym");
    if (id_to_idx.count(id_str))
      return "ERROR DUPLICATE: Element id '" + id_str + "' already exists\n";
    if (!symbols.count(sym_id))
      return "ERROR NOT_FOUND: Symbol '" + sym_id + "' does not exist\n";
    Element use_elem;
    use_elem.id          = id_str;
    use_elem.type        = "USE";
    use_elem.grp         = params.count("grp") ? params.at("grp") : "root";
    use_elem.pos_first   = (params.count("pos") && params.at("pos") == "f");
    use_elem.class_names = params.count("class") ? params.at("class") : "";
    use_elem.attrs["sym"] = sym_id;
    if (params.count("x"))  use_elem.attrs["x"]  = params.at("x");
    if (params.count("y"))  use_elem.attrs["y"]  = params.at("y");
    if (params.count("w"))  use_elem.attrs["w"]  = params.at("w");
    if (params.count("h"))  use_elem.attrs["h"]  = params.at("h");
    if (params.count("tr")) use_elem.transform   = parse_transform(params.at("tr"));
    for (auto& kv : params) {
      if (kv.first == "id" || kv.first == "sym" || kv.first == "grp" ||
          kv.first == "pos" || kv.first == "class" || kv.first == "tr" ||
          kv.first == "x"   || kv.first == "y"   ||
          kv.first == "w"   || kv.first == "h") continue;
      use_elem.attrs[kv.first] = kv.second;
    }
    int parent_idx = -1;
    if (use_elem.grp != "root") {
      auto it = id_to_idx.find(use_elem.grp);
      if (it != id_to_idx.end() && !elements[it->second].deleted)
        parent_idx = it->second;
    }
    use_elem.parent_idx = parent_idx;
    int new_idx = (int)elements.size();
    elements.push_back(use_elem);
    id_to_idx[id_str] = new_idx;
    dirty = true;
    return "OK\n";
  }

  std::string update_attributes(const std::unordered_map<std::string, std::string>& params) {
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    const std::string& id_str = params.at("id");
    auto it = id_to_idx.find(id_str);
    if (it == id_to_idx.end() || elements[it->second].deleted)
      return "ERROR NOT_FOUND: Element '" + id_str + "' does not exist\n";
    Element& elem = elements[it->second];
    for (auto& kv : params) {
      if (kv.first == "id") continue;
      if (kv.first == "tr") {
        elem.transform = parse_transform(kv.second);
      } else if (kv.first == "class") {
        elem.class_names = kv.second;
      } else {
        if (kv.first == "href" && elem.type == "IMG") {
          auto ic = image_cache.find(elem.id);
          if (ic != image_cache.end()) { delete ic->second; image_cache.erase(ic); }
          std::string err = load_image(elem.id, kv.second);
          if (!err.empty()) return err;
        }
        elem.attrs[kv.first] = kv.second;
      }
    }
    dirty = true;
    return "OK\n";
  }

  std::string delete_elements(const std::unordered_map<std::string, std::string>& params) {
    if (params.count("all") && params.at("all") == "t") {
      for (auto& kv : image_cache) delete kv.second;
      image_cache.clear();
      elements.clear();
      id_to_idx.clear();
      children.clear();
      dirty = true;
      return "OK\n";
    }
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    std::string ids = params.at("id");
    size_t start = 0;
    while (start < ids.size()) {
      size_t end = ids.find(',', start);
      if (end == std::string::npos) end = ids.size();
      std::string one_id = ids.substr(start, end - start);
      if (!one_id.empty()) {
        auto it = id_to_idx.find(one_id);
        if (it != id_to_idx.end() && !elements[it->second].deleted) {
          elements[it->second].deleted = true;
          auto ic = image_cache.find(one_id);
          if (ic != image_cache.end()) { delete ic->second; image_cache.erase(ic); }
          id_to_idx.erase(it);
        }
      }
      start = end + 1;
    }
    dirty = true;
    return "OK\n";
  }

  std::string get_elements(const std::unordered_map<std::string, std::string>& params) {
    auto format_elem = [](const Element& e) {
      std::string r = "OK id=" + e.id + " type=" + e.type;
      r += " grp=" + e.grp;
      r += std::string(" pos=") + (e.pos_first ? "f" : "l");
      for (auto& kv : e.attrs) {
        if (kv.first == "href") r += " href=[data]";
        else r += " " + kv.first + "=" + kv.second;
      }
      if (!e.class_names.empty()) r += " class=" + e.class_names;
      return r + "\n";
    };
    if (params.count("all") && params.at("all") == "t") {
      std::string resp;
      for (auto& e : elements) if (!e.deleted) resp += format_elem(e);
      return resp.empty() ? "OK\n" : resp;
    }
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    std::string ids = params.at("id");
    std::string resp;
    size_t start = 0;
    while (start < ids.size()) {
      size_t end = ids.find(',', start);
      if (end == std::string::npos) end = ids.size();
      std::string one_id = ids.substr(start, end - start);
      if (!one_id.empty()) {
        auto it = id_to_idx.find(one_id);
        if (it == id_to_idx.end() || elements[it->second].deleted)
          resp += "ERROR NOT_FOUND: Element '" + one_id + "' does not exist\n";
        else
          resp += format_elem(elements[it->second]);
      }
      start = end + 1;
    }
    return resp;
  }

  std::string exists_element(const std::unordered_map<std::string, std::string>& params) {
    if (!params.count("id")) return "ERROR SYNTAX: Missing id parameter\n";
    auto it = id_to_idx.find(params.at("id"));
    bool exists = (it != id_to_idx.end() && !elements[it->second].deleted);
    return std::string("OK EX=") + (exists ? "t" : "f") + "\n";
  }

  std::string set_style(const std::unordered_map<std::string, std::string>& params) {
    if (!params.count("sel")) return "ERROR SYNTAX: Missing sel parameter\n";
    auto& rule = styles[params.at("sel")];
    for (auto& kv : params) {
      if (kv.first != "sel") rule[kv.first] = kv.second;
    }
    dirty = true;
    return "OK\n";
  }

  std::string delete_styles(const std::unordered_map<std::string, std::string>& params) {
    if (params.count("all") && params.at("all") == "t") {
      styles.clear();
      dirty = true;
      return "OK\n";
    }
    if (!params.count("sel")) return "ERROR SYNTAX: Missing sel parameter\n";
    std::string sels = params.at("sel");
    size_t start = 0;
    while (start < sels.size()) {
      size_t end = sels.find(',', start);
      if (end == std::string::npos) end = sels.size();
      std::string sel = sels.substr(start, end - start);
      if (!sel.empty()) styles.erase(sel);
      start = end + 1;
    }
    dirty = true;
    return "OK\n";
  }

  std::string get_styles(const std::unordered_map<std::string, std::string>& params) {
    auto format_rule = [](const std::string& sel,
                          const std::unordered_map<std::string, std::string>& props) {
      std::string r = "OK sel=" + sel;
      for (auto& kv : props) r += " " + kv.first + "=" + kv.second;
      return r + "\n";
    };
    if (params.count("all") && params.at("all") == "t") {
      if (styles.empty()) return "OK\n";
      std::string resp;
      for (auto& kv : styles) resp += format_rule(kv.first, kv.second);
      return resp;
    }
    if (!params.count("sel")) return "ERROR SYNTAX: Missing sel parameter\n";
    auto it = styles.find(params.at("sel"));
    if (it == styles.end()) return "ERROR NOT_FOUND: Selector not found\n";
    return format_rule(it->first, it->second);
  }
};
