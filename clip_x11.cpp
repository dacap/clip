// Clip Library
// Copyright (c) 2018 David Capello

#include "clip.h"
#include "clip_x11.h"

#include <xcb/xcb.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace clip {

class Manager {
public:
  typedef std::shared_ptr<std::vector<uint8_t>> buffer_ptr;
  typedef std::vector<xcb_atom_t> atoms;

  Manager()
    : m_connection(xcb_connect(nullptr, nullptr))
    , m_stop(false) {
    xcb_screen_t* screen =
      xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;
    m_window = xcb_generate_id(m_connection);
    xcb_create_window(m_connection, 0,
                      m_window,
                      screen->root,
                      0, 0, 1, 1, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      0, nullptr);

    m_thread = std::thread(
      [this]{
        process_x11_events();
      });
  }

  ~Manager() {
    // TODO remove this and create some method to keep the clipboard alive.
    std::this_thread::sleep_for(std::chrono::seconds(10));

    m_stop = true;
    m_thread.join();

    xcb_destroy_window(m_connection, m_window);
    xcb_disconnect(m_connection);
  }

  bool try_lock() { return m_mutex.try_lock(); }
  void unlock() { m_mutex.unlock(); }

  void clear() {
    m_data.clear();
  }

  bool is_convertible(format f) const {
    if (f == text_format()) {
      xcb_window_t owner = get_x11_selection_owner();
      if (owner == m_window) {
        for (xcb_atom_t atom : get_text_format_atoms()) {
          auto it = m_data.find(atom);
          if (it != m_data.end())
            return true;
        }
      }
      else {
        // TODO ask atoms to selection owner
      }
    }
    return false;
  }

  bool set_data(format f, const char* buf, size_t len) {
    if (!set_x11_selection_owner())
      return false;

    if (f == text_format()) {
      buffer_ptr shared_text_buf = std::make_shared<std::vector<uint8_t>>(len);
      std::copy(buf,
                buf+len,
                shared_text_buf->begin());
      for (xcb_atom_t atom : get_text_format_atoms())
        m_data[atom] = shared_text_buf;
      return true;
    }
    return false;
  }

  bool get_data(format f, char* buf, size_t len) const {
    if (f == text_format()) {
      xcb_window_t owner = get_x11_selection_owner();
      if (owner == m_window) {
        for (xcb_atom_t atom : get_text_format_atoms()) {
          auto it = m_data.find(atom);
          if (it != m_data.end()) {
            std::copy(it->second->begin(),
                      it->second->begin()+std::min(len, it->second->size()),
                      buf);
            return true;
          }
        }
      }
      else {
        // TODO ask data content to selection owner
      }
    }
    return false;
  }

  size_t get_data_length(format f) const {
    if (f == text_format()) {
      xcb_window_t owner = get_x11_selection_owner();
      if (owner == m_window) {
        for (xcb_atom_t atom : get_text_format_atoms()) {
          auto it = m_data.find(atom);
          if (it != m_data.end())
            return it->second->size();
        }
      }
      else {
        // TODO ask data length to selection owner
      }
    }
    return 0;
  }

private:

  void process_x11_events() {
    xcb_generic_event_t* event;
    while (!m_stop) {
      while ((event = xcb_poll_for_event(m_connection))) {
        int type = (event->response_type & ~0x80);
        switch (type) {

          // Someone else has new content in the clipboard, so is
          // notifying us that we should delete our data now.
          case XCB_SELECTION_CLEAR:
            handle_selection_clear_event(
              (xcb_selection_clear_event_t*)event);
            break;

          // Someone is requesting the clipboard content from us.
          case XCB_SELECTION_REQUEST:
            handle_selection_request_event(
              (xcb_selection_request_event_t*)event);
            break;

          // We've requested the clipboard content and this is the
          // answer.
          case XCB_SELECTION_NOTIFY:
            handle_selection_notify_event(
              (xcb_selection_notify_event_t*)event);
            break;
        }

        free(event);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void handle_selection_clear_event(xcb_selection_clear_event_t* event) {
    if (event->selection == get_atom("CLIPBOARD")) {
      std::lock_guard<std::mutex> lock(m_mutex);
      clear(); // Clear our clipboard data
    }
  }

  void handle_selection_request_event(xcb_selection_request_event_t* event) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (event->target == get_atom("TARGETS")) {
      atoms targets;
      for (const auto& it : m_data)
        targets.push_back(it.first);

      // Set the "property" of "requestor" with the clipboard
      // formats ("targets", atoms) that we provide.
      xcb_change_property(
        m_connection,
        XCB_PROP_MODE_REPLACE,
        event->requestor,
        event->property,
        event->target,
        8*sizeof(xcb_atom_t),
        targets.size(),
        &targets[0]);
    }
    else {
      auto it = m_data.find(event->target);
      if (it != m_data.end()) {
        // Set the "property" of "requestor" with the
        // clipboard content in the requested format ("target").
        xcb_change_property(
          m_connection,
          XCB_PROP_MODE_REPLACE,
          event->requestor,
          event->property,
          event->target,
          8,
          it->second->size(),
          &(*it->second)[0]);
      }
    }

    // Notify the "requestor" that we've already updated the property.
    xcb_selection_notify_event_t notify;
    notify.response_type = XCB_SELECTION_NOTIFY;
    notify.pad0          = 0;
    notify.sequence      = 0;
    notify.time          = XCB_CURRENT_TIME;
    notify.requestor     = event->requestor;
    notify.selection     = event->selection;
    notify.target        = event->target;
    notify.property      = event->property;
    xcb_send_event(m_connection, false,
                   event->requestor,
                   XCB_EVENT_MASK_PROPERTY_CHANGE,
                   (const char*)&notify);

    xcb_flush(m_connection);
  }

  void handle_selection_notify_event(xcb_selection_notify_event_t* event) {
    // TODO
  }

  atoms get_atoms(const char** names,
                  const int n) const {
    atoms result(n, 0);
    std::vector<xcb_intern_atom_cookie_t> cookies(n);

    for (int i=0; i<n; ++i) {
      auto it = m_atoms.find(names[i]);
      if (it != m_atoms.end())
        result[i] = it->second;
      else
        cookies[i] = xcb_intern_atom(
          m_connection, 0,
          std::strlen(names[i]), names[i]);
    }

    for (int i=0; i<n; ++i) {
      if (result[i] == 0) {
        xcb_intern_atom_reply_t* reply =
          xcb_intern_atom_reply(m_connection,
                                cookies[i],
                                nullptr);
        if (reply) {
          result[i] = m_atoms[names[i]] = reply->atom;
          free(reply);
        }
      }
    }

    return result;
  }

  xcb_atom_t get_atom(const char* name) const {
    auto it = m_atoms.find(name);
    if (it != m_atoms.end())
      return it->second;

    xcb_atom_t result = 0;
    xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(m_connection, 0,
                      std::strlen(name), name);

    xcb_intern_atom_reply_t* reply =
      xcb_intern_atom_reply(m_connection,
                            cookie,
                            nullptr);
    if (reply) {
      result = m_atoms[name] = reply->atom;
      free(reply);
    }
    return result;
  }

  const atoms& get_text_format_atoms() const {
    if (m_text_atoms.empty()) {
      const char* names[] = {
        "UTF8_STRING",
        "STRING",
        "TEXT",
        "text/plain",
        "text/plain;charset=utf-8",
        "text/plain;charset=UTF-8"
      };
      m_text_atoms = get_atoms(names, sizeof(names) / sizeof(names[0]));
    }
    return m_text_atoms;
  }

  bool set_x11_selection_owner() const {
    xcb_void_cookie_t cookie =
      xcb_set_selection_owner_checked(m_connection,
                                      m_window,
                                      get_atom("CLIPBOARD"),
                                      XCB_CURRENT_TIME);
    xcb_generic_error_t* err =
      xcb_request_check(m_connection,
                        cookie);
    if (err) {
      free(err);
      return false;
    }
    return true;
  }

  xcb_window_t get_x11_selection_owner() const {
    xcb_window_t result = 0;
    xcb_get_selection_owner_cookie_t cookie =
      xcb_get_selection_owner(m_connection,
                              get_atom("CLIPBOARD"));

    xcb_get_selection_owner_reply_t* reply =
      xcb_get_selection_owner_reply(m_connection, cookie, nullptr);
    if (reply) {
      result = reply->owner;
      free(reply);
    }
    return result;
  }

  xcb_connection_t* m_connection;
  xcb_window_t m_window;
  bool m_stop;
  std::thread m_thread;
  mutable std::map<std::string, xcb_atom_t> m_atoms; // Cache of known atoms
  mutable atoms m_text_atoms;                        // Cache of atoms related to text content
  std::map<xcb_atom_t, buffer_ptr> m_data;           // Actual clipboard data
  std::mutex m_mutex;
};

static Manager* manager = nullptr;
static void delete_manager_atexit() {
  if (manager) {
    delete manager;
    manager = nullptr;
  }
}

lock::impl::impl(void*) : m_locked(false) {
  if (!manager) {
    manager = new Manager;
    std::atexit(delete_manager_atexit);
  }
  m_locked = manager->try_lock();
}

lock::impl::~impl() {
  if (m_locked)
    manager->unlock();
}

bool lock::impl::locked() const {
  return m_locked;
}

bool lock::impl::clear() {
  manager->clear();
  return true;
}

bool lock::impl::is_convertible(format f) const {
  return manager->is_convertible(f);
}

bool lock::impl::set_data(format f, const char* buf, size_t len) {
  return manager->set_data(f, buf, len);
}

bool lock::impl::get_data(format f, char* buf, size_t len) const {
  return manager->get_data(f, buf, len);
}

size_t lock::impl::get_data_length(format f) const {
  return manager->get_data_length(f);
}

bool lock::impl::set_image(const image& image) {
  return false;                 // TODO
}

bool lock::impl::get_image(image& output_img) const {
  return false;                 // TODO
}

bool lock::impl::get_image_spec(image_spec& spec) const {
  return false;                 // TODO
}

format register_format(const std::string& name) {
  return 0;                     // TODO
}

} // namespace clip
