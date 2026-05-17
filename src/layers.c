#include <wlr/types/wlr_layer_shell_v1.h>

#include "focus.h"
#include "layout.h"
#include "output.h"
#include "types.h"

/* arrange layers logic from dwl */

static void layer_arrange(absn_output *output, struct wl_list *list,
                          struct wlr_box *usable_area, int exclusive)
{
    absn_layer_surface *layer_surface;
    wl_list_for_each(layer_surface, list, link)
    {
        struct wlr_layer_surface_v1 *surface = layer_surface->wlr;

        if (!surface->initialized) {
            continue;
        }

        if (exclusive != (surface->current.exclusive_zone > 0)) {
            continue;
        }

        wlr_scene_layer_surface_v1_configure(layer_surface->scene_layer,
                                             &output->geom, usable_area);
        wlr_scene_node_set_position(&layer_surface->popups->node,
                                    layer_surface->scene_tree->node.x,
                                    layer_surface->scene_tree->node.y);
    }
}

void layers_arrange(absn_output *output)
{
    if (!output->wlr->enabled) {
        return;
    }

    struct wlr_box usable_area = output->usable_area;

    /* Arrange non-exclusive surfaces from top->bottom */
    for (int i = 3; i >= 0; --i) {
        layer_arrange(output, &output->layers[i], &usable_area, 1);
    }

    if (!wlr_box_equal(&output->usable_area, &usable_area)) {
        output->usable_area = usable_area;
        output_arrange(output);
    }

    /* Arrange non-exclusive surfaces from top->bottom */
    for (int i = 3; i >= 0; --i) {
        layer_arrange(output, &output->layers[i], &usable_area, 0);
    }

    uint32_t layers_above_shell[] = {
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP,
    };

    int layers_above_shell_count =
        sizeof(layers_above_shell) / sizeof(layers_above_shell[0]);
    absn_layer_surface *layer_surface;
    for (int i = 0; i < layers_above_shell_count; ++i) {
        wl_list_for_each_reverse(layer_surface,
                                 &output->layers[layers_above_shell[i]], link)
        {
            if (!layer_surface->wlr->current.keyboard_interactive ||
                !layer_surface->mapped) {
                continue;
            }

            unfocus_toplevel(output->server->focused_toplevel);
            output->server->exclusive_focus = layer_surface;
            struct wlr_keyboard *keyboard =
                wlr_seat_get_keyboard(output->server->seat);
            if (keyboard) {
                wlr_seat_keyboard_notify_enter(
                    output->server->seat, layer_surface->wlr->surface,
                    keyboard->keycodes, keyboard->num_keycodes,
                    &keyboard->modifiers);
            }
            return;
        }
    }
}
