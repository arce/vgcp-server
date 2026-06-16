#pragma once
#include <string>
#include <FL/fl_draw.H>

#ifndef fl_rgba_color
#define fl_rgba_color(r,g,b,a) \
  ((((unsigned int)(r))<<24)|(((unsigned int)(g))<<16)|(((unsigned int)(b))<<8)|((unsigned int)(a)))
#endif

inline Fl_Color parse_color(const std::string& s) {
  if (s.empty() || s == "none") return fl_rgba_color(0, 0, 0, 0);
  if (s[0] != '#') {
    if (s == "red")     return FL_RED;
    if (s == "blue")    return FL_BLUE;
    if (s == "green")   return FL_GREEN;
    if (s == "yellow")  return FL_YELLOW;
    if (s == "black")   return FL_BLACK;
    if (s == "white")   return FL_WHITE;
    if (s == "orange")  return fl_rgb_color(255, 165,   0);
    if (s == "cyan")    return fl_rgb_color(  0, 255, 255);
    if (s == "magenta") return fl_rgb_color(255,   0, 255);
    if (s == "pink")    return fl_rgb_color(255, 192, 203);
    if (s == "gold")    return fl_rgb_color(255, 215,   0);
    if (s == "purple")  return fl_rgb_color(128,   0, 128);
    if (s == "gray")    return fl_rgb_color(128, 128, 128);
    if (s == "grey")    return fl_rgb_color(128, 128, 128);
    if (s == "brown")   return fl_rgb_color(165,  42,  42);
    if (s == "lime")    return fl_rgb_color(  0, 255,   0);
    if (s == "navy")    return fl_rgb_color(  0,   0, 128);
    return FL_BLACK;
  }
  try {
    std::string hex = s.substr(1);
    if (hex.size() == 3)
      hex = {hex[0], hex[0], hex[1], hex[1], hex[2], hex[2]};
    if (hex.size() < 6) return FL_BLACK;
    auto x = [&](size_t off) {
      return (uchar)std::stoul(hex.substr(off, 2), nullptr, 16);
    };
    uchar r = x(0), g = x(2), b = x(4);
    // FLTK RGB format: 0xRRGGBB00 — last byte must be 0.
    // fl_rgba_color(0,0,0,255) = 255 = indexed color (not black!).
    // fl_rgb_color handles the special case: (0,0,0) → FL_BLACK = 0.
    if (hex.size() >= 8) {
      uchar a = x(6);
      return fl_rgba_color(r, g, b, a == 255 ? 0 : a);
    }
    return fl_rgb_color(r, g, b);
  } catch (...) { return FL_BLACK; }
}
