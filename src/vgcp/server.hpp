#pragma once
#include <string>
#include <algorithm>
#include <cstring>

// ── Platform abstraction ───────────────────────────────────────────────────
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  inline int sock_read(int fd, void* buf, size_t len) {
    return recv((SOCKET)fd, (char*)buf, (int)len, 0);
  }
  inline int sock_write(int fd, const void* buf, size_t len) {
    return send((SOCKET)fd, (const char*)buf, (int)len, 0);
  }
  inline void sock_close(int fd) { closesocket((SOCKET)fd); }
  inline void set_nonblocking(int fd) {
    u_long mode = 1;
    ioctlsocket((SOCKET)fd, FIONBIO, &mode);
  }
  // FLTK on Windows defines Fl_FD_Handler as void(*)(unsigned long long, void*)
  typedef unsigned long long fltk_fd_t;
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/stat.h>
  #include <netinet/in.h>
  #include <fcntl.h>
  inline int sock_read(int fd, void* buf, size_t len)        { return (int)read(fd, buf, len); }
  inline int sock_write(int fd, const void* buf, size_t len) { return (int)write(fd, buf, len); }
  inline void sock_close(int fd)      { close(fd); }
  inline void set_nonblocking(int fd) { fcntl(fd, F_SETFL, O_NONBLOCK); }
  typedef int fltk_fd_t;
#endif

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

inline void data_cb(fltk_fd_t fd_native, void* data) {
  int fd = (int)fd_native;
  ClientSession* session = static_cast<ClientSession*>(data);
  char buf[8192];
  int bytes = sock_read(fd, buf, sizeof(buf));
  if (bytes <= 0) {
    if (session->is_pipe) return;
    Fl::remove_fd(fd);
    sock_close(fd);
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
    if (!response.empty()) sock_write(fd, response.data(), response.size());
    if (response == "BYE\n" && !session->is_pipe) {
      Fl::remove_fd(fd);
      sock_close(fd);
      g_client_fds.erase(std::remove(g_client_fds.begin(), g_client_fds.end(), fd), g_client_fds.end());
      delete session;
      return;
    }
  }
  sync_window();
}

inline void server_accept_cb(fltk_fd_t server_fd_native, void*) {
  int server_fd = (int)server_fd_native;
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
#ifdef _WIN32
  SOCKET cs = accept((SOCKET)server_fd, (struct sockaddr*)&addr, &len);
  if (cs == INVALID_SOCKET) return;
  int client_fd = (int)cs;
#else
  int client_fd = accept(server_fd, (struct sockaddr*)&addr, &len);
  if (client_fd < 0) return;
#endif
  set_nonblocking(client_fd);
  g_client_fds.push_back(client_fd);
  Fl::add_fd(client_fd, FL_READ, data_cb, new ClientSession{client_fd, "", false});
}
