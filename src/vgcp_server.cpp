#include <FL/Fl_Double_Window.H>
#include <cstring>
#include <cstdlib>
#include "vgcp/server.hpp"

std::vector<int> g_client_fds;
VGCPDocument doc;
VGCPWidget* global_widget = nullptr;
Fl_Window* global_window  = nullptr;

int main(int argc, char* argv[]) {
  Mode mode = Mode::SOCKET;
  std::string pipe_path = "/tmp/vgcp_pipe";
  int port = 3891;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
      mode = (std::string(argv[++i]) == "pipe") ? Mode::PIPE : Mode::SOCKET;
    } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      try { port = std::stoi(argv[++i]); } catch (...) {}
    } else if (strcmp(argv[i], "--pipe") == 0 && i + 1 < argc) {
      pipe_path = argv[++i];
    } else if (mode == Mode::SOCKET) {
      try { port = std::stoi(argv[i]); } catch (...) {}
    } else {
      pipe_path = argv[i];
    }
  }

  int listen_fd = -1;
  ClientSession* pipe_session = nullptr;

  if (mode == Mode::SOCKET) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return 1;
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;
    if (listen(listen_fd, 10) < 0) return 1;
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);
    Fl::add_fd(listen_fd, FL_READ, server_accept_cb, nullptr);
  } else {
    unlink(pipe_path.c_str());
    if (mkfifo(pipe_path.c_str(), 0666) < 0) return 1;
    listen_fd = open(pipe_path.c_str(), O_RDWR | O_NONBLOCK);
    if (listen_fd < 0) return 1;
    pipe_session = new ClientSession{listen_fd, "", true};
    Fl::add_fd(listen_fd, FL_READ, data_cb, pipe_session);
  }

  global_window = new Fl_Double_Window(doc.viewport_width, doc.viewport_height,
                                       "VGCP Server Renderer v3.3");
  global_window->color(FL_BLACK);
  global_widget = new VGCPWidget(0, 0, doc.viewport_width, doc.viewport_height);
  global_window->resizable(global_widget);
  global_window->end();
  global_window->show();

  int result = Fl::run();

  close(listen_fd);
  if (mode == Mode::PIPE) {
    unlink(pipe_path.c_str());
    delete pipe_session;
  }
  return result;
}
