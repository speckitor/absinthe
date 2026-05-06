#include <stdlib.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/util/log.h>

#include "output.h"
#include "seat.h"
#include "server.h"
#include "types.h"

static int
setup(absn_server *server)
{
	wlr_log_init(WLR_DEBUG, NULL);
	server->display = wl_display_create();
	if (!server->display) {
		wlr_log(WLR_ERROR, "Failed to create wl_display");
		return 1;
	}

	server->backend = wlr_backend_autocreate(wl_display_get_event_loop(
						     server->display),
	    NULL);
	if (!server->backend) {
		wlr_log(WLR_ERROR, "Failed to create wlr_backend");
		return 1;
	}

	server->renderer = wlr_renderer_autocreate(server->backend);
	if (!server->renderer) {
		wlr_log(WLR_ERROR, "Failed to create wlr_renderer");
		return 1;
	}

	wlr_renderer_init_wl_shm(server->renderer, server->display);

	server->scene = wlr_scene_create();
	if (wlr_renderer_get_texture_formats(server->renderer,
		WLR_BUFFER_CAP_DMABUF)) {
		wlr_drm_create(server->display, server->renderer);
		wlr_scene_set_linux_dmabuf_v1(server->scene,
		    wlr_linux_dmabuf_v1_create_with_renderer(server->display, 5,
			server->renderer));
	}

	server->allocator = wlr_allocator_autocreate(server->backend,
	    server->renderer);
	if (!server->allocator) {
		wlr_log(WLR_ERROR, "Failed to create wlr_allocator");
		return 1;
	}

	/* wlroots managers */
	server->compositor = wlr_compositor_create(server->display, 6,
	    server->renderer);
	wlr_subcompositor_create(server->display);
	wlr_data_device_manager_create(server->display);
	wlr_screencopy_manager_v1_create(server->display);
	wlr_data_control_manager_v1_create(server->display);
	wlr_viewporter_create(server->display);
	wlr_single_pixel_buffer_manager_v1_create(server->display);
	wlr_fractional_scale_manager_v1_create(server->display, 1);
	wlr_presentation_create(server->display, server->backend, 2);
	wlr_alpha_modifier_v1_create(server->display);
	wlr_export_dmabuf_manager_v1_create(server->display);
	wlr_ext_foreign_toplevel_list_v1_create(server->display, 1);

	wlr_server_decoration_manager_set_default_mode(
	    wlr_server_decoration_manager_create(server->display),
	    WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);
	server->xdg_deco_mgr = wlr_xdg_decoration_manager_v1_create(
	    server->display);
	LISTEN(server->new_xdg_deco, new_xdg_decoration,
	    server->xdg_deco_mgr->events.new_toplevel_decoration);

	/* output */
	wl_list_init(&server->outputs);
	LISTEN(server->new_output, new_output,
	    server->backend->events.new_output);

	server->output_layout = wlr_output_layout_create(server->display);
	LISTEN(server->layout_change, output_layout_change,
	    server->output_layout->events.change);

	server->output_mgr = wlr_output_manager_v1_create(server->display);

	wlr_xdg_output_manager_v1_create(server->display,
	    server->output_layout);

	server->scene_layout = wlr_scene_attach_output_layout(server->scene,
	    server->output_layout);

	/* toplevels lists */
	wl_list_init(&server->toplevels);
	wl_list_init(&server->focus_stack);

	/* xdg_shell */
	server->xdg_shell = wlr_xdg_shell_create(server->display, 6);
	LISTEN(server->new_xdg_toplevel, new_xdg_toplevel,
	    server->xdg_shell->events.new_toplevel);
	LISTEN(server->new_xdg_popup, new_xdg_popup,
	    server->xdg_shell->events.new_popup);

	/* cursor setup */
	server->cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

	server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

	server->cursor_mode = CURSOR_PASSTHROUGH;
	LISTEN(server->cursor_motion, cursor_motion,
	    server->cursor->events.motion);
	LISTEN(server->cursor_motion_abs, cursor_motion_abs,
	    server->cursor->events.motion_absolute);
	LISTEN(server->cursor_button, cursor_button,
	    server->cursor->events.button);
	LISTEN(server->cursor_axis, cursor_axis, server->cursor->events.axis);
	LISTEN(server->cursor_frame, cursor_frame,
	    server->cursor->events.frame);

	/* keyboard */
	wl_list_init(&server->keyboards);
	server->seat = wlr_seat_create(server->display, "seat0");
	LISTEN(server->new_input, new_input, server->backend->events.new_input);
	LISTEN(server->request_cursor, request_cursor,
	    server->seat->events.request_set_cursor);
	LISTEN(server->pointer_focus_change, pointer_focus_change,
	    server->seat->pointer_state.events.focus_change);
	LISTEN(server->request_set_selection, request_cursor,
	    server->seat->events.request_set_selection);

	unsetenv("DISPLAY");
#ifdef XWAYLAND
	if ((server->xwayland = wlr_xwayland_create(server->display,
		 server->compositor, 1))) {
		LISTEN(server->xw_ready, xwayland_ready,
		    server->xwayland->events.ready);
		LISTEN(server->xw_new_surface, xwayland_new_surface,
		    server->xwayland->events.new_surface);

		setenv("DISPLAY", server->xwayland->display_name, 1);
		wlr_log(WLR_INFO, "Running XWayland, DISPLAY=%s",
		    server->xwayland->display_name);
	} else {
		wlr_log(WLR_ERROR,
		    "Failed to setup XWayland, continuing without it");
	}
#endif

	const char *socket = wl_display_add_socket_auto(server->display);
	if (!socket) {
		wlr_log(WLR_ERROR, "Failed to add socket");
		wlr_backend_destroy(server->backend);
		return 1;
	}

	if (!wlr_backend_start(server->backend)) {
		wlr_log(WLR_ERROR, "Failed to start wlr_backend");
		wlr_backend_destroy(server->backend);
		wl_display_destroy(server->display);
		return 1;
	}

	setenv("WAYLAND_DISPLAY", socket, true);
	wlr_log(WLR_INFO, "Running absinthe on WAYLAND_DISPLAY=%s", socket);

	return 0;
}

static void
cleanup(absn_server *server)
{
	wl_display_destroy_clients(server->display);

	wl_list_remove(&server->new_xdg_toplevel.link);
	wl_list_remove(&server->new_xdg_popup.link);
	wl_list_remove(&server->new_xdg_deco.link);

	wl_list_remove(&server->cursor_motion.link);
	wl_list_remove(&server->cursor_motion_abs.link);
	wl_list_remove(&server->cursor_button.link);
	wl_list_remove(&server->cursor_axis.link);
	wl_list_remove(&server->cursor_frame.link);

	wl_list_remove(&server->new_input.link);
	wl_list_remove(&server->request_cursor.link);
	wl_list_remove(&server->pointer_focus_change.link);
	wl_list_remove(&server->request_set_selection.link);

	wl_list_remove(&server->new_output.link);

	wlr_scene_node_destroy(&server->scene->tree.node);
	wlr_xcursor_manager_destroy(server->cursor_mgr);
	wlr_cursor_destroy(server->cursor);
	wlr_allocator_destroy(server->allocator);
	wlr_renderer_destroy(server->renderer);
	wlr_backend_destroy(server->backend);
	wl_display_destroy(server->display);
}

int
main(void)
{
	absn_server server = { 0 };
	int err = setup(&server);
	if (err)
		return EXIT_FAILURE;
	wl_display_run(server.display);
	cleanup(&server);
	return EXIT_SUCCESS;
}
