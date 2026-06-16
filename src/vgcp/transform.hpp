#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "utils.hpp"

struct Transform {
  double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;

  Transform() = default;
  Transform(double a_, double b_, double c_, double d_, double e_, double f_)
    : a(a_), b(b_), c(c_), d(d_), e(e_), f(f_) {}

  Transform multiply(const Transform& o) const {
    return Transform(
      a * o.a + b * o.c,
      a * o.b + b * o.d,
      c * o.a + d * o.c,
      c * o.b + d * o.d,
      e * o.a + f * o.c + o.e,
      e * o.b + f * o.d + o.f
    );
  }

  void apply(double& x, double& y) const {
    double nx = a * x + c * y + e;
    double ny = b * x + d * y + f;
    x = nx; y = ny;
  }
};

inline Transform parse_transform(const std::string& s) {
  if (s.empty() || s == "none") return Transform();
  Transform result;
  size_t pos = 0;
  while (pos < s.size()) {
    while (pos < s.size() && s[pos] == ' ') pos++;
    if (pos >= s.size()) break;
    std::string func;
    while (pos < s.size() && (isalpha(s[pos]) || s[pos] == 'X' || s[pos] == 'Y'))
      func += s[pos++];
    while (pos < s.size() && s[pos] == ' ') pos++;
    if (pos >= s.size() || s[pos] != '(') return Transform();
    pos++;
    std::vector<double> args;
    while (pos < s.size() && s[pos] != ')') {
      while (pos < s.size() && s[pos] == ' ') pos++;
      if (pos >= s.size() || s[pos] == ')') break;
      size_t end = pos;
      while (end < s.size() && s[end] != ',' && s[end] != ')' && s[end] != ' ') end++;
      args.push_back(parse_double(s.substr(pos, end - pos)));
      pos = end;
      while (pos < s.size() && (s[pos] == ' ' || s[pos] == ',')) pos++;
    }
    if (pos < s.size() && s[pos] == ')') pos++;
    Transform t;
    if (func == "matrix" && args.size() == 6) {
      t = Transform(args[0], args[1], args[2], args[3], args[4], args[5]);
    } else if (func == "t" && args.size() >= 1) {
      double tx = args[0], ty = (args.size() > 1) ? args[1] : 0;
      t = Transform(1, 0, 0, 1, tx, ty);
    } else if (func == "r" && args.size() >= 1) {
      double angle = args[0] * M_PI / 180.0;
      double cosA = cos(angle), sinA = sin(angle);
      if (args.size() == 3) {
        double cx = args[1], cy = args[2];
        t = Transform(cosA, sinA, -sinA, cosA,
                      cx - cosA * cx + sinA * cy,
                      cy - sinA * cx - cosA * cy);
      } else {
        t = Transform(cosA, sinA, -sinA, cosA, 0, 0);
      }
    } else if (func == "s" && args.size() >= 1) {
      double sx = args[0], sy = (args.size() > 1) ? args[1] : sx;
      t = Transform(sx, 0, 0, sy, 0, 0);
    } else if (func == "skewX" && args.size() >= 1) {
      double angle = args[0] * M_PI / 180.0;
      t = Transform(1, 0, tan(angle), 1, 0, 0);
    } else if (func == "skewY" && args.size() >= 1) {
      double angle = args[0] * M_PI / 180.0;
      t = Transform(1, tan(angle), 0, 1, 0, 0);
    }
    result = t.multiply(result);
  }
  return result;
}
