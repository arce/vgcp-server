#pragma once
#include <sstream>
#include <cmath>
#include <FL/fl_draw.H>
#include "document.hpp"

namespace {

struct PathSeg {
  char type = 'L';
  double x = 0, y = 0;
  double cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0;
  double from_x = 0, from_y = 0;
};

class SvgPathParser {
  const std::string& d_;
  size_t pos_;
  void skip() {
    while (pos_ < d_.size() && (d_[pos_] == ' ' || d_[pos_] == ',' ||
                                  d_[pos_] == '\t' || d_[pos_] == '\n'))
      ++pos_;
  }
public:
  explicit SvgPathParser(const std::string& d) : d_(d), pos_(0) {}
  bool has_next() { skip(); return pos_ < d_.size(); }
  bool is_cmd()   { skip(); return pos_ < d_.size() && isalpha((unsigned char)d_[pos_]); }
  char read_cmd() { skip(); return d_[pos_++]; }
  double read_num() {
    skip();
    size_t s = pos_;
    if (pos_ < d_.size() && (d_[pos_] == '+' || d_[pos_] == '-')) ++pos_;
    while (pos_ < d_.size() && (isdigit((unsigned char)d_[pos_]) || d_[pos_] == '.')) ++pos_;
    if (pos_ < d_.size() && (d_[pos_] == 'e' || d_[pos_] == 'E')) {
      ++pos_;
      if (pos_ < d_.size() && (d_[pos_] == '+' || d_[pos_] == '-')) ++pos_;
      while (pos_ < d_.size() && isdigit((unsigned char)d_[pos_])) ++pos_;
    }
    try { return std::stod(d_.substr(s, pos_ - s)); } catch (...) { return 0.0; }
  }
};

inline std::vector<std::vector<PathSeg>> parse_svg_path(const std::string& d) {
  std::vector<std::vector<PathSeg>> subpaths;
  std::vector<PathSeg> current;

  SvgPathParser p(d);
  char cmd = 'M', prev_cmd = 0;
  double cx = 0, cy = 0, sx = 0, sy = 0;
  double last_cp_x = 0, last_cp_y = 0;

  while (p.has_next()) {
    if (p.is_cmd()) cmd = p.read_cmd();

    char C   = (char)toupper((unsigned char)cmd);
    bool rel = (cmd != C);

    auto abs_pt = [&](double dx, double dy, double& ax, double& ay) {
      ax = rel ? cx + dx : dx;
      ay = rel ? cy + dy : dy;
    };

    if (C == 'M') {
      double ax, ay; abs_pt(p.read_num(), p.read_num(), ax, ay);
      if (!current.empty()) { subpaths.push_back(current); current.clear(); }
      current.push_back({'M', ax, ay});
      cx = sx = ax; cy = sy = ay;
      cmd = rel ? 'l' : 'L';
    }
    else if (C == 'L') {
      double ax, ay; abs_pt(p.read_num(), p.read_num(), ax, ay);
      current.push_back({'L', ax, ay});
      cx = ax; cy = ay;
    }
    else if (C == 'H') {
      double ax = rel ? cx + p.read_num() : p.read_num();
      current.push_back({'L', ax, cy});
      cx = ax;
    }
    else if (C == 'V') {
      double ay = rel ? cy + p.read_num() : p.read_num();
      current.push_back({'L', cx, ay});
      cy = ay;
    }
    else if (C == 'C') {
      double x1, y1, x2, y2, x, y;
      abs_pt(p.read_num(), p.read_num(), x1, y1);
      abs_pt(p.read_num(), p.read_num(), x2, y2);
      abs_pt(p.read_num(), p.read_num(), x,  y);
      current.push_back({'C', x, y, x1, y1, x2, y2, cx, cy});
      last_cp_x = x2; last_cp_y = y2;
      cx = x; cy = y;
    }
    else if (C == 'S') {
      double x2, y2, x, y;
      abs_pt(p.read_num(), p.read_num(), x2, y2);
      abs_pt(p.read_num(), p.read_num(), x,  y);
      char pc = (char)toupper((unsigned char)prev_cmd);
      double x1 = (pc == 'C' || pc == 'S') ? 2*cx - last_cp_x : cx;
      double y1 = (pc == 'C' || pc == 'S') ? 2*cy - last_cp_y : cy;
      current.push_back({'C', x, y, x1, y1, x2, y2, cx, cy});
      last_cp_x = x2; last_cp_y = y2;
      cx = x; cy = y;
    }
    else if (C == 'Q') {
      double qx, qy, x, y;
      abs_pt(p.read_num(), p.read_num(), qx, qy);
      abs_pt(p.read_num(), p.read_num(), x,  y);
      double x1 = cx + 2.0/3.0*(qx - cx), y1 = cy + 2.0/3.0*(qy - cy);
      double x2 = x  + 2.0/3.0*(qx - x),  y2 = y  + 2.0/3.0*(qy - y);
      current.push_back({'C', x, y, x1, y1, x2, y2, cx, cy});
      last_cp_x = qx; last_cp_y = qy;
      cx = x; cy = y;
    }
    else if (C == 'T') {
      double x, y;
      abs_pt(p.read_num(), p.read_num(), x, y);
      char pc = (char)toupper((unsigned char)prev_cmd);
      double qx = (pc == 'Q' || pc == 'T') ? 2*cx - last_cp_x : cx;
      double qy = (pc == 'Q' || pc == 'T') ? 2*cy - last_cp_y : cy;
      double x1 = cx + 2.0/3.0*(qx - cx), y1 = cy + 2.0/3.0*(qy - cy);
      double x2 = x  + 2.0/3.0*(qx - x),  y2 = y  + 2.0/3.0*(qy - y);
      current.push_back({'C', x, y, x1, y1, x2, y2, cx, cy});
      last_cp_x = qx; last_cp_y = qy;
      cx = x; cy = y;
    }
    else if (C == 'A') {
      p.read_num(); p.read_num(); p.read_num();
      p.read_num(); p.read_num();
      double x, y; abs_pt(p.read_num(), p.read_num(), x, y);
      current.push_back({'L', x, y});
      cx = x; cy = y;
    }
    else if (C == 'Z') {
      current.push_back({'Z', sx, sy});
      cx = sx; cy = sy;
      subpaths.push_back(current);
      current.clear();
    }

    prev_cmd = cmd;
  }

  if (!current.empty()) subpaths.push_back(current);
  return subpaths;
}

} // anonymous namespace

inline void VGCPDocument::rebuild_hierarchy() {
  if (!dirty) return;
  children.clear();
  for (size_t i = 0; i < elements.size(); ++i) {
    if (elements[i].deleted) continue;
    int pidx = elements[i].parent_idx;
    if (pidx != -1 && elements[pidx].deleted) pidx = -1;
    if (pidx != -1 && !elements[pidx].deleted) {
      if (elements[i].pos_first)
        children[pidx].insert(children[pidx].begin(), (int)i);
      else
        children[pidx].push_back((int)i);
    } else {
      if (elements[i].pos_first)
        children[-1].insert(children[-1].begin(), (int)i);
      else
        children[-1].push_back((int)i);
    }
  }
  dirty = false;
}

inline std::unordered_map<std::string, std::string> VGCPDocument::resolve_style(
    const Element& elem,
    const std::unordered_map<std::string, std::string>& inherited) {
  std::unordered_map<std::string, std::string> props = inherited;
  if (styles.count(elem.type)) {
    for (auto& kv : styles.at(elem.type)) props[kv.first] = kv.second;
  }
  std::string classes = elem.class_names;
  size_t pos = 0;
  while (pos < classes.size()) {
    while (pos < classes.size() && classes[pos] == ' ') pos++;
    if (pos >= classes.size()) break;
    size_t end = classes.find(' ', pos);
    if (end == std::string::npos) end = classes.size();
    std::string cls = classes.substr(pos, end - pos);
    if (styles.count("." + cls)) {
      for (auto& kv : styles.at("." + cls)) props[kv.first] = kv.second;
    }
    pos = end;
  }
  if (styles.count("#" + elem.id)) {
    for (auto& kv : styles.at("#" + elem.id)) props[kv.first] = kv.second;
  }
  for (auto& kv : elem.attrs) {
    if (kv.first == "class") continue;
    if (kv.first == "w") props["width"]  = kv.second;
    else if (kv.first == "h") props["height"] = kv.second;
    else props[kv.first] = kv.second;
  }
  double parent_op = 1.0;
  auto op_it = inherited.find("op");
  if (op_it != inherited.end()) parent_op = parse_double(op_it->second, 1.0);
  auto op_it2 = props.find("op");
  if (op_it2 != props.end()) {
    double child_op = parse_double(op_it2->second, 1.0);
    props["op"] = std::to_string(parent_op * child_op);
  } else {
    props["op"] = std::to_string(parent_op);
  }
  return props;
}

inline void VGCPDocument::draw_symbol_inst(
    const std::string& sym_id,
    const std::unordered_map<std::string, std::string>& overrides,
    const Transform& parent_tr,
    double offset_x, double offset_y,
    double override_w, double override_h) {
  auto it = symbols.find(sym_id);
  if (it == symbols.end()) return;
  Symbol& sym = it->second;
  double vx, vy, vw, vh;
  std::istringstream vb_ss(sym.viewbox);
  if (!(vb_ss >> vx >> vy >> vw >> vh)) return;
  double scale_x = (override_w > 0 && vw > 0) ? override_w / vw : 1.0;
  double scale_y = (override_h > 0 && vh > 0) ? override_h / vh : 1.0;
  Transform sym_tr = Transform(scale_x, 0, 0, scale_y, offset_x, offset_y).multiply(parent_tr);
  for (auto& elem : sym.content) {
    draw_element(elem, overrides, sym_tr);
  }
}

inline void VGCPDocument::draw_element(
    const Element& elem,
    const std::unordered_map<std::string, std::string>& inherited,
    const Transform& parent_tr) {
  if (elem.deleted) return;
  Transform total_tr = elem.transform.multiply(parent_tr);
  std::unordered_map<std::string, std::string> props = resolve_style(elem, inherited);

  auto get = [&](const std::string& k, const std::string& def = "") -> std::string {
    auto it = props.find(k);
    return (it != props.end()) ? it->second : def;
  };

  Fl_Color fill_color   = parse_color(get("fill",   "none"));
  Fl_Color stroke_color = parse_color(get("stroke", "none"));
  int sw = parse_int(get("sw", "0"));

  fl_push_matrix();
  fl_mult_matrix(total_tr.a, total_tr.b, total_tr.c, total_tr.d, total_tr.e, total_tr.f);

  if (elem.type == "RCT") {
    int x  = parse_int(get("x")),  y  = parse_int(get("y"));
    int w  = parse_int(get("w", "100")), h = parse_int(get("h", "100"));
    int rx = parse_int(get("rx", "0")),  ry = parse_int(get("ry", "0"));
    { std::string fv = get("fill"); if (!fv.empty() && fv != "none") {
      fl_color(fill_color);
      if (rx > 0 || ry > 0) {
        int r2 = std::min(rx, ry); if (r2 == 0) r2 = std::max(rx, ry);
        fl_begin_polygon();
        fl_arc((double)(x + r2),     (double)(y + r2),     (double)r2,  90.0, 180.0);
        fl_arc((double)(x + w - r2), (double)(y + r2),     (double)r2,   0.0,  90.0);
        fl_arc((double)(x + w - r2), (double)(y + h - r2), (double)r2, 270.0, 360.0);
        fl_arc((double)(x + r2),     (double)(y + h - r2), (double)r2, 180.0, 270.0);
        fl_end_polygon();
      } else {
        fl_rectf(x, y, w, h);
      }
    }}
    if (sw > 0 && get("stroke") != "none") {
      fl_color(stroke_color);
      fl_line_style(FL_SOLID, sw);
      if (rx > 0 || ry > 0) {
        int r2 = std::min(rx, ry); if (r2 == 0) r2 = std::max(rx, ry);
        fl_begin_loop();
        fl_arc((double)(x + r2),     (double)(y + r2),     (double)r2,  90.0, 180.0);
        fl_arc((double)(x + w - r2), (double)(y + r2),     (double)r2,   0.0,  90.0);
        fl_arc((double)(x + w - r2), (double)(y + h - r2), (double)r2, 270.0, 360.0);
        fl_arc((double)(x + r2),     (double)(y + h - r2), (double)r2, 180.0, 270.0);
        fl_end_loop();
      } else {
        fl_rect(x, y, w, h);
      }
      fl_line_style(0);
    }
  }
  else if (elem.type == "CIR") {
    int cx = parse_int(get("cx")), cy = parse_int(get("cy"));
    int r  = parse_int(get("r", "10"));
    if (get("fill") != "none") {
      fl_color(fill_color);
      fl_pie(cx - r, cy - r, r * 2, r * 2, 0, 360);
    }
    if (sw > 0 && get("stroke") != "none") {
      fl_color(stroke_color);
      fl_line_style(FL_SOLID, sw);
      fl_arc(cx - r, cy - r, r * 2, r * 2, 0, 360);
      fl_line_style(0);
    }
  }
  else if (elem.type == "ELL") {
    int cx = parse_int(get("cx")), cy = parse_int(get("cy"));
    int rx = parse_int(get("rx", "10")), ry = parse_int(get("ry", "10"));
    if (get("fill") != "none") {
      fl_color(fill_color);
      fl_pie(cx - rx, cy - ry, rx * 2, ry * 2, 0, 360);
    }
    if (sw > 0 && get("stroke") != "none") {
      fl_color(stroke_color);
      fl_line_style(FL_SOLID, sw);
      fl_arc(cx - rx, cy - ry, rx * 2, ry * 2, 0, 360);
      fl_line_style(0);
    }
  }
  else if (elem.type == "LIN") {
    int x1 = parse_int(get("x1")), y1 = parse_int(get("y1"));
    int x2 = parse_int(get("x2")), y2 = parse_int(get("y2"));
    if (sw > 0 && get("stroke") != "none") {
      fl_color(stroke_color);
      fl_line_style(FL_SOLID, std::max(1, sw));
      fl_line(x1, y1, x2, y2);
      fl_line_style(0);
    }
  }
  else if (elem.type == "TXT") {
    int x = parse_int(get("x")), y = parse_int(get("y"));
    std::string content = get("txt");
    std::string ff   = get("ff", "Helvetica");
    bool mono = (ff.find("Courier") != std::string::npos || ff.find("mono") != std::string::npos);
    bool bold = (get("fw") == "b");
    Fl_Font font = mono ? (bold ? FL_COURIER_BOLD : FL_COURIER)
                        : (bold ? FL_HELVETICA_BOLD : FL_HELVETICA);
    int fs   = parse_int(get("fs", "14"));
    std::string ta = get("ta", "s");
    fl_font(font, fs);
    fl_color(fill_color);
    int tw = (int)fl_width(content.c_str());
    int dx = (ta == "e") ? -tw : (ta == "m") ? -tw / 2 : 0;
    fl_draw(content.c_str(), x + dx, y);
  }
  else if (elem.type == "PAT") {
    std::string d = get("d");
    if (!d.empty()) {
      auto subpaths = parse_svg_path(d);

      auto render = [&](bool is_fill) {
        for (auto& sp : subpaths) {
          if (sp.size() < 2) continue;
          if (is_fill) fl_begin_polygon();
          else         fl_begin_line();
          for (auto& seg : sp) {
            if (seg.type == 'C')
              fl_curve(seg.from_x, seg.from_y,
                       seg.cx1, seg.cy1, seg.cx2, seg.cy2,
                       seg.x, seg.y);
            else
              fl_vertex(seg.x, seg.y);
          }
          if (is_fill) fl_end_polygon();
          else         fl_end_line();
        }
      };

      std::string fv = get("fill");
      if (!fv.empty() && fv != "none") {
        fl_color(fill_color);
        render(true);
      }
      if (sw > 0 && get("stroke") != "none") {
        fl_color(stroke_color);
        fl_line_style(FL_SOLID, sw);
        render(false);
        fl_line_style(0);
      }
    }
  }
  else if (elem.type == "PGN" || elem.type == "PLN") {
    std::string pts_str = get("pts");
    std::vector<std::pair<int, int>> pts;
    std::string tmp = pts_str;
    for (char& c : tmp) if (c == ',') c = ' ';
    std::istringstream ss(tmp);
    double pa, pb;
    while (ss >> pa >> pb) pts.emplace_back((int)pa, (int)pb);
    if (pts.size() >= 2) {
      { std::string fv = get("fill"); if (elem.type == "PGN" && !fv.empty() && fv != "none") {
        fl_color(fill_color);
        fl_begin_polygon();
        for (auto& p : pts) fl_vertex(p.first, p.second);
        fl_end_polygon();
      }}
      if (sw > 0 || elem.type == "PLN") {
        fl_color(stroke_color);
        fl_line_style(FL_SOLID, std::max(1, sw));
        fl_begin_line();
        for (auto& p : pts) fl_vertex(p.first, p.second);
        if (elem.type == "PGN") fl_vertex(pts[0].first, pts[0].second);
        fl_end_line();
        fl_line_style(0);
      }
    }
  }
  else if (elem.type == "IMG") {
    auto img_it = image_cache.find(elem.id);
    if (img_it != image_cache.end()) {
      int x = parse_int(get("x")), y = parse_int(get("y"));
      int w = parse_int(get("w", std::to_string(img_it->second->w())));
      int h = parse_int(get("h", std::to_string(img_it->second->h())));
      if (w != img_it->second->w() || h != img_it->second->h()) {
        Fl_RGB_Image* scaled = (Fl_RGB_Image*)img_it->second->copy(w, h);
        scaled->draw(x, y);
        delete scaled;
      } else {
        img_it->second->draw(x, y);
      }
    }
  }
  fl_pop_matrix();

  if (elem.type == "USE") {
    std::string sym_id = get("sym");
    double ux = parse_double(get("x")), uy = parse_double(get("y"));
    double uw = parse_double(get("w")), uh = parse_double(get("h"));
    std::unordered_map<std::string, std::string> overrides_map;
    for (auto& kv : elem.attrs) {
      if (kv.first == "sym" || kv.first == "x" || kv.first == "y" ||
          kv.first == "w"   || kv.first == "h") continue;
      overrides_map[kv.first] = kv.second;
    }
    draw_symbol_inst(sym_id, overrides_map, total_tr, ux, uy, uw, uh);
  }

  auto child_it = children.find(
    id_to_idx.count(elem.id) ? id_to_idx.at(elem.id) : -2);
  if (child_it != children.end()) {
    for (int child_idx : child_it->second) {
      draw_element(elements[child_idx], props, total_tr);
    }
  }
}

inline void VGCPDocument::draw() {
  rebuild_hierarchy();
  for (int root_idx : children[-1]) {
    draw_element(elements[root_idx], {}, Transform());
  }
}

inline std::string VGCPDocument::find_element_at(int px, int py) {
  for (int i = (int)elements.size() - 1; i >= 0; --i) {
    if (elements[i].deleted) continue;
    const Element& e = elements[i];
    auto get = [&](const std::string& k, const std::string& def = "") {
      auto it = e.attrs.find(k);
      return (it != e.attrs.end()) ? it->second : def;
    };
    bool hit = false;
    if (e.type == "RCT") {
      int x = parse_int(get("x")), y = parse_int(get("y"));
      int w = parse_int(get("w", "100")), h = parse_int(get("h", "100"));
      hit = (px >= x && px <= x + w && py >= y && py <= y + h);
    } else if (e.type == "CIR") {
      int cx = parse_int(get("cx")), cy = parse_int(get("cy"));
      int r  = parse_int(get("r", "10"));
      int dx = px - cx, dy = py - cy;
      hit = (dx * dx + dy * dy <= r * r);
    } else if (e.type == "ELL") {
      int cx = parse_int(get("cx")), cy = parse_int(get("cy"));
      int rx = parse_int(get("rx", "10")), ry = parse_int(get("ry", "10"));
      double fx = (double)(px - cx) / rx, fy = (double)(py - cy) / ry;
      hit = (fx * fx + fy * fy <= 1.0);
    } else if (e.type == "TXT") {
      int x = parse_int(get("x")), y = parse_int(get("y"));
      int fs = parse_int(get("fs", "14"));
      hit = (px >= x && px <= x + 100 && py >= y - fs && py <= y);
    }
    if (hit) return e.id;
  }
  return "";
}
