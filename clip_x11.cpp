// Clip Library
// Copyright (c) 2018 David Capello

#include "clip.h"
#include "clip_x11.h"

#include <xcb/xcb.h>

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace clip {

namespace {

enum CommonAtom {
  CLIPBOARD,
  TARGETS,
};

const char* kCommonAtomNames[] = {
  "CLIPBOARD",
  "TARGETS",
};

class Manager {
public:
  typedef std::shared_ptr<std::vector<uint8_t>> buffer_ptr;
  typedef std::vector<xcb_atom_t> atoms;
  typedef std::function<bool()> notify_callback;

  Manager()
    : m_connection(xcb_connect(nullptr, nullptr))
    , m_stop(false)
    , m_reply(nullptr) {
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
    if (!m_data.empty() &&
        m_window &&
        m_window == get_x11_selection_owner()) {
      // TODO remove this and create some method to keep the clipboard alive.
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    m_stop = true;
    m_thread.join();

    xcb_destroy_window(m_connection, m_window);
    xcb_disconnect(m_connection);
  }

  bool try_lock() {
    return m_mutex.try_lock();
  }

  void unlock() {
    destroy_get_clipboard_reply();
    m_mutex.unlock();
  }

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
        return true;
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
            int n = std::min(len, it->second->size());
            std::copy(it->second->begin(),
                      it->second->begin()+n,
                      buf);

            // Add an extra null char
            if (n < len)
              (*it->second)[n++] = 0;

            return true;
          }
        }
      }
      else if (owner) {
        if (get_data_from_selection_owner(
              get_text_format_atoms(),
              [this, buf, len]() -> bool {
                uint8_t* src = (uint8_t*)xcb_get_property_value(m_reply);
                size_t srclen = xcb_get_property_value_length(m_reply);
                srclen = srclen * (m_reply->format/8);

                size_t n = std::min(srclen, len);
                std::copy(src, src+n, buf);
                if (n < len)
                  buf[n] = 0; // Include a null character

                return true;
              })) {
          return false;
        }
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
            return it->second->size()+1; // Add an extra byte for the null char
        }
      }
      else if (owner) {
        size_t len = 0;
        if (get_data_from_selection_owner(
              get_text_format_atoms(),
              [this, &len]() -> bool {
                len = xcb_get_property_value_length(m_reply);
                len = len * (m_reply->format/8);

                if (len > 0)
                  ++len;          // For the null character
                return true;
              })) {
          return len;
        }
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
    if (event->selection == get_atom(CLIPBOARD)) {
      std::lock_guard<std::mutex> lock(m_mutex);
      clear(); // Clear our clipboard data
    }
  }

  void handle_selection_request_event(xcb_selection_request_event_t* event) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (event->target == get_atom(TARGETS)) {
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
      else {
        // Nothing to do (unsupported target)
        return;
      }
    }

    // Notify the "requestor" that we've already updated the property.
    xcb_selection_notify_event_t notify;
    notify.response_type = XCB_SELECTION_NOTIFY;
    notify.pad0          = 0;
    notify.sequence      = 0;
    notify.time          = event->time;
    notify.requestor     = event->requestor;
    notify.selection     = event->selection;
    notify.target        = event->target;
    notify.property      = event->property;

    xcb_send_event(m_connection, false,
                   event->requestor,
                   XCB_EVENT_MASK_NO_EVENT, // SelectionNotify events go without mask
                   (const char*)&notify);

    xcb_flush(m_connection);
  }

  void handle_selection_notify_event(xcb_selection_notify_event_t* event) {
    destroy_get_clipboard_reply();

    assert(event->requestor == m_window);

    xcb_get_property_cookie_t cookie =
      xcb_get_property(m_connection,
                       true,
                       event->requestor,
                       event->property,
                       event->target,
                       0, 0x1fffffff); // 0x1fffffff = INT32_MAX / 4

    xcb_generic_error_t* err = nullptr;
    m_reply = xcb_get_property_reply(m_connection, cookie, &err);

    if (err) {
      free(err);
    }

    if (m_reply) {
      if (m_callback)
        m_callback();
      m_cv.notify_one();
    }
  }

  void destroy_get_clipboard_reply() {
    if (m_reply) {
      free(m_reply);
      m_reply = nullptr;
    }
  }

  bool get_data_from_selection_owner(const atoms& atoms,
                                     const notify_callback&& callback) const {
    // Used cached reply (e.g. after a get_data_length, we can re-use
    // the reply just to get the data itself)
    if (m_reply) {
      assert(callback);
      if (callback)
        callback();
      return true;
    }

    // Put the callback on "m_callback" so we can call it on
    // SelectionNotify event.
    m_callback = std::move(callback);
    m_data.clear();

    std::unique_lock<std::mutex> lock(m_mutex, std::adopt_lock);

    // Ask to the selection owner for its content on each known
    // text format/atom.
    for (xcb_atom_t atom : atoms) {
      xcb_convert_selection(m_connection,
                            m_window, // Send us the result
                            get_atom(CLIPBOARD), // Clipboard selection
                            atom, // The clipboard format that we're requesting
                            get_atom(CLIPBOARD), // Leave result in this window's property
                            XCB_CURRENT_TIME);

      xcb_flush(m_connection);

      // Wait a response for 100 milliseconds
      std::cv_status status =
        m_cv.wait_for(lock, std::chrono::milliseconds(100));
      if (status == std::cv_status::no_timeout) {
        return true;
      }
    }

    // Reset callback
    m_callback = notify_callback();
    return false;
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

  xcb_atom_t get_atom(CommonAtom i) const {
    if (m_common_atoms.empty()) {
      m_common_atoms =
        get_atoms(kCommonAtomNames,
                  sizeof(kCommonAtomNames) / sizeof(kCommonAtomNames[0]));
    }
    return m_common_atoms[i];
  }

  const atoms& get_text_format_atoms() const {
    if (m_text_atoms.empty()) {
      const char* names[] = {
        // Prefer utf-8 formats first
        "UTF8_STRING",
        "text/plain;charset=utf-8",
        "text/plain;charset=UTF-8"
        // ANSI C strings?
        "STRING",
        "TEXT",
        "text/plain",
      };
      m_text_atoms = get_atoms(names, sizeof(names) / sizeof(names[0]));
    }
    return m_text_atoms;
  }

  bool set_x11_selection_owner() const {
    xcb_void_cookie_t cookie =
      xcb_set_selection_owner_checked(m_connection,
                                      m_window,
                                      get_atom(CLIPBOARD),
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
                              get_atom(CLIPBOARD));

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

  // Thread used to run a background message loop to wait X11 events
  // about clipboard. The X11 selection owner will be a hidden window
  // created by us just for the clipboard purpose/communication.
  std::thread m_thread;

  // Internal callback used when a SelectionNotify is received. So
  // this callback can use the notification by different purposes
  // (e.g. get the data length only, or get the data content).
  mutable notify_callback m_callback;

  // Cache of known atoms
  mutable std::map<std::string, xcb_atom_t> m_atoms;

  // Cache of common used atoms by us
  mutable atoms m_common_atoms;

  // Cache of atoms related to text content
  mutable atoms m_text_atoms;

  // Actual clipboard data generated by us (when we "copy" content in
  // the clipboard, it means that we own the X11 "CLIPBOARD"
  // selection, and in case of SelectionRequest events, we've to
  // return the data stored in this "m_data" field)
  mutable std::map<xcb_atom_t, buffer_ptr> m_data;

  // Access to the whole Manager
  mutable std::mutex m_mutex;

  // Used to wait/notify the arrival of the SelectionNotify event when
  // we requested the clipboard content from other selection owner.
  mutable std::condition_variable m_cv;

  // Cached reply for xcb_get_property_reply() when we want to get the
  // clipboard data content/length from the current selection owner.
  // As general rule we want to make two consecutive calls
  // (get_data_length, and then get_data) so we can use the same
  // xcb_get_property_reply_t for both queries.
  mutable xcb_get_property_reply_t* m_reply;
};

Manager* manager = nullptr;
void delete_manager_atexit() {
  if (manager) {
    delete manager;
    manager = nullptr;
  }
}

} // anonymous namespace

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
