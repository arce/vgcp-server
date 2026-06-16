#pragma once
#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include "renderer.hpp"
#include "globals.hpp"

class VGCPWidget : public Fl_Widget {
public:
  VGCPWidget(int x, int y, int w, int h) : Fl_Widget(x, y, w, h) {}

  void draw() override {
    fl_push_clip(x(), y(), w(), h());
    fl_color(parse_color(doc.bg_color));
    fl_rectf(x(), y(), w(), h());
    doc.draw();
    fl_pop_clip();
  }

  int handle(int event) override {
    static std::string last_enter_id;
    int lx = Fl::event_x() - x();
    int ly = Fl::event_y() - y();

    auto send_event = [&](const std::string& type, const std::string& target) {
      std::string msg = "EVENT type=" + type +
                        " target=" + target +
                        " x=" + std::to_string(lx) +
                        " y=" + std::to_string(ly) +
                        " gx=" + std::to_string(Fl::event_x()) +
                        " gy=" + std::to_string(Fl::event_y()) + "\n";
      broadcast_event(msg);
    };

    switch (event) {
      case FL_PUSH: {
        std::string id = doc.find_element_at(lx, ly);
        if (!id.empty()) {
          std::string btn = (Fl::event_button() == FL_LEFT_MOUSE)  ? "left"
                          : (Fl::event_button() == FL_RIGHT_MOUSE) ? "right"
                                                                    : "middle";
          if (Fl::event_clicks() > 0) {
            send_event("dblclick", id);
          } else {
            std::string msg = "EVENT type=mousedown target=" + id +
                              " x=" + std::to_string(lx) +
                              " y=" + std::to_string(ly) +
                              " gx=" + std::to_string(Fl::event_x()) +
                              " gy=" + std::to_string(Fl::event_y()) +
                              " button=" + btn + "\n";
            broadcast_event(msg);
          }
        }
        return 1;
      }
      case FL_RELEASE: {
        std::string id = doc.find_element_at(lx, ly);
        if (!id.empty()) {
          std::string btn = (Fl::event_button() == FL_LEFT_MOUSE)  ? "left"
                          : (Fl::event_button() == FL_RIGHT_MOUSE) ? "right"
                                                                    : "middle";
          std::string msg = "EVENT type=mouseup target=" + id +
                            " x=" + std::to_string(lx) +
                            " y=" + std::to_string(ly) +
                            " gx=" + std::to_string(Fl::event_x()) +
                            " gy=" + std::to_string(Fl::event_y()) +
                            " button=" + btn + "\n";
          broadcast_event(msg);
          std::string click_msg = "EVENT type=click target=" + id +
                                  " x=" + std::to_string(lx) +
                                  " y=" + std::to_string(ly) +
                                  " gx=" + std::to_string(Fl::event_x()) +
                                  " gy=" + std::to_string(Fl::event_y()) +
                                  " button=" + btn + "\n";
          broadcast_event(click_msg);
        }
        return 1;
      }
      case FL_MOVE:
      case FL_DRAG: {
        std::string id = doc.find_element_at(lx, ly);
        if (!id.empty()) send_event("mousemove", id);
        if (id != last_enter_id) {
          if (!last_enter_id.empty()) send_event("mouseleave", last_enter_id);
          if (!id.empty())            send_event("mouseenter", id);
          last_enter_id = id;
        }
        return 1;
      }
      default:
        return Fl_Widget::handle(event);
    }
  }
};
