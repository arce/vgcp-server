#pragma once
#include <vector>
#include <string>
#include <unistd.h>

class VGCPDocument;
class VGCPWidget;
class Fl_Window;

extern std::vector<int> g_client_fds;
extern VGCPDocument doc;
extern VGCPWidget* global_widget;
extern Fl_Window* global_window;

inline void broadcast_event(const std::string& msg) {
  for (int fd : g_client_fds) {
    write(fd, msg.data(), msg.size());
  }
}
