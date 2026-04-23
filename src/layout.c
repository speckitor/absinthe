#include "absinthe-toplevel.h"
#include "types.h"

void layout_arrange(struct absinthe_output *output)
{
    if (!output)
        return;

    struct absinthe_toplevel *toplevel;
    size_t toplevels_count = 0;
    wl_list_for_each(toplevel, &output->server->toplevels, link) {
        if (toplevel->tiled && toplevel->output == output) {
            toplevels_count++;
            wlr_scene_node_set_enabled(&toplevel->scene_tree->node, true);
        }
    }

    if (toplevels_count < 1)
        return;

    int32_t output_gap = ABSINTHE_OUTPUT_GAP;

    if (toplevels_count == 1) {
        wl_list_for_each(toplevel, &output->server->toplevels, link) {
            if (toplevel->output == output)
                break;
        }
        toplevel->geometry.x = output->geometry.x + output_gap;
        toplevel->geometry.y = output->geometry.y + output_gap;
        absinthe_toplevel_set_size(toplevel,
                                   output->geometry.width - 2 * output_gap,
                                   output->geometry.height - 2 * output_gap);
        return;
    }

    int32_t layout_gap = ABSINTHE_LAYOUT_GAP;
    int32_t main_stack_width = (toplevels_count <= output->main_stack_size)
        ? output->geometry.width - 2 * output_gap
        : output->main_stack_width * (output->geometry.width - 2 * output_gap);
    int32_t width = output->geometry.width - main_stack_width - 2 * output_gap - layout_gap;
    int32_t height;
    int32_t dy = output_gap;
    size_t i = 0;

    if (toplevels_count <= output->main_stack_size) {
        wl_list_for_each(toplevel, &output->server->toplevels, link) {
            if (toplevel->output != output || !toplevel->tiled)
                continue;

            height = (output->geometry.height - dy - output_gap) / (toplevels_count - i);
            toplevel->geometry.x = output->geometry.x + output_gap;
            toplevel->geometry.y = output->geometry.y + dy;
            absinthe_toplevel_set_size(toplevel, main_stack_width, height);
            dy += height + layout_gap;

            i++;
        }
        return;
    }

    wl_list_for_each(toplevel, &output->server->toplevels, link) {
        if (toplevel->output != output || !toplevel->tiled)
            continue;

        if (i < output->main_stack_size) {
            height = (output->geometry.height - dy - output_gap) / (output->main_stack_size - i);
            toplevel->geometry.x = output->geometry.x + output_gap;
            toplevel->geometry.y = output->geometry.y + dy;
            absinthe_toplevel_set_size(toplevel, main_stack_width, height);
            dy += height + layout_gap;
        } else {
            if (i == output->main_stack_size)
                dy = output_gap;
            height = (output->geometry.height - dy - output_gap) / (toplevels_count - i);
            toplevel->geometry.x = output->geometry.x + main_stack_width + layout_gap + output_gap;
            toplevel->geometry.y = output->geometry.y + dy;
            absinthe_toplevel_set_size(toplevel, width, height);
            dy += height + layout_gap;
        }

        i++;
    }
}
