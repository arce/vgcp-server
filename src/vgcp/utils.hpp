#pragma once
#include <string>
#include <vector>

inline int parse_int(const std::string& s, int def = 0) {
  if (s.empty()) return def;
  try { return std::stoi(s); } catch (...) { return def; }
}

inline double parse_double(const std::string& s, double def = 0.0) {
  if (s.empty()) return def;
  try { return std::stod(s); } catch (...) { return def; }
}

inline std::vector<unsigned char> base64_decode(const std::string& in) {
  static const std::string table =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::vector<unsigned char> out;
  out.reserve(in.size() * 3 / 4);
  int val = 0, bits = -8;
  for (unsigned char c : in) {
    if (c == '=') break;
    auto p = table.find(c);
    if (p == std::string::npos) continue;
    val = (val << 6) | (int)p;
    bits += 6;
    if (bits >= 0) {
      out.push_back((val >> bits) & 0xFF);
      bits -= 8;
    }
  }
  return out;
}
