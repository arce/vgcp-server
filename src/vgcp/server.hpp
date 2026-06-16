#pragma once
#include <string>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <FL/Fl.H>
#include "widget.hpp"

enum class Mode { SOCKET, PIPE };

struct ClientSession {
  int fd;
  std::string buffer;
  bool is_pipe;
};

inline void sync_window() {
  if (!global_window || !global_widget) return;
  if (global_window->w() != doc.viewport_width ||
      global_window->h() != doc.viewport_height) {
    global_window->size(doc.viewport_width, doc.viewport_height);
    global_widget->size(doc.viewport_width, doc.viewport_height);
  }
  global_widget->redraw();
}

inline void data_cb(int fd, void* data) {
  ClientSession* session = static_cast<ClientSession*>(data);
  char buf[8192];
  ssize_t bytes = read(fd, buf, sizeof(buf));
  if (bytes <= 0) {
    if (session->is_pipe) return;
    Fl::remove_fd(fd);
    close(fd);
    g_client_fds.erase(std::remove(g_client_fds.begin(), g_client_fds.end(), fd), g_client_fds.end());
    delete session;
    return;
  }
  session->buffer.append(buf, bytes);
  size_t pos;
  while ((pos = session->buffer.find('\n')) != std::string::npos) {
    std::string line = session->buffer.substr(0, pos);
    session->buffer.erase(0, pos + 1);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::string response = doc.process_command(line);
    if (!response.empty()) write(fd, response.data(), response.size());
    if (response == "BYE\n" && !session->is_pipe) {
      Fl::remove_fd(fd);
      close(fd);
      g_client_fds.erase(std::remove(g_client_fds.begin(), g_client_fds.end(), fd), g_client_fds.end());
      delete session;
      return;
    }
  }
  sync_window();
}

inline void server_accept_cb(int server_fd, void*) {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int client_fd = accept(server_fd, (struct sockaddr*)&addr, &len);
  if (client_fd < 0) return;
  fcntl(client_fd, F_SETFL, O_NONBLOCK);
  g_client_fds.push_back(client_fd);
  Fl::add_fd(client_fd, FL_READ, data_cb, new ClientSession{client_fd, "", false});
}
