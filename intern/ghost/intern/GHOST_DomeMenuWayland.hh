/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <cstdlib>
#include <cstring>
#include <wayland-client.h>
#include "dome-appmenu-client-protocol.h"

/* A private registry queue avoids dispatching GHOST window/input callbacks from
 * inside construction. Only explicitly launched DomeD instances export menus. */
struct GHOST_DomeMenuWayland {
  org_kde_kwin_appmenu_manager *manager = nullptr;
  org_kde_kwin_appmenu *menu = nullptr;
  static void global(void *data, wl_registry *registry, uint32_t name,
                     const char *interface, uint32_t version)
  {
    auto *self = static_cast<GHOST_DomeMenuWayland *>(data);
    if (version >= 1 && std::strcmp(interface, "org_kde_kwin_appmenu_manager") == 0) {
      self->manager = static_cast<org_kde_kwin_appmenu_manager *>(
          wl_registry_bind(registry, name, &org_kde_kwin_appmenu_manager_interface,
                           version >= 2 ? 2 : 1));
    }
  }
  static void removed(void *, wl_registry *, uint32_t) {}
  void attach(wl_display *display, wl_surface *surface)
  {
    const char *service = std::getenv("DOME_MENU_SERVICE");
    const char *path = std::getenv("DOME_MENU_PATH");
    if (!service || !*service || !path || !*path) return;
    wl_event_queue *queue = wl_display_create_queue(display);
    if (!queue) return;
    wl_registry *registry = wl_display_get_registry(display);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(registry), queue);
    static const wl_registry_listener listener = {global, removed};
    wl_registry_add_listener(registry, &listener, this);
    if (wl_display_roundtrip_queue(display, queue) >= 0 && manager) {
      menu = org_kde_kwin_appmenu_manager_create(manager, surface);
      org_kde_kwin_appmenu_set_address(menu, service, path);
      wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(menu), nullptr);
      setenv("BLENDER_DOME_MENU_ATTACHED", "1", 1);
    }
    if (manager) {
      if (org_kde_kwin_appmenu_manager_get_version(manager) >= 2)
        org_kde_kwin_appmenu_manager_release(manager);
      else
        org_kde_kwin_appmenu_manager_destroy(manager);
      manager = nullptr;
    }
    wl_registry_destroy(registry);
    wl_event_queue_destroy(queue);
  }
  void detach()
  {
    if (menu) org_kde_kwin_appmenu_release(menu);
    menu = nullptr;
  }
};
