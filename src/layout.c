#include "types.h"

void layout_arrange(struct absinthe_output *output)
{
    if (!output)
        return;

    struct absinthe_toplevel *toplevel;
    size_t toplevels_count = 0;
    wl_list_for_each(toplevel, &output->server->toplevels, link) {
        if (toplevel->output == output)
            toplevels_count++;
    }


    if (toplevels_count < 1)
        return;

    int32_t borders_width = ABSINTHE_TOPLEVEL_BORDER_WIDTH * 2;
    int32_t output_gap = ABSINTHE_OUTPUT_GAP;

    if (toplevels_count == 1) {
        wl_list_for_each(toplevel, &output->server->toplevels, link) {
            if (toplevel->output == output)
                break;
        }

        wlr_xdg_toplevel_set_size(toplevel->toplevel.xdg,
                                  output->geometry.width - borders_width - 2 * output_gap,
                                  output->geometry.height - borders_width - 2 * output_gap);
        toplevel->geometry.x = output->geometry.x + output_gap;
        toplevel->geometry.y = output->geometry.y + output_gap;
        return;
    }

    int32_t layout_gap = ABSINTHE_LAYOUT_GAP;
    int32_t main_toplevel_width = ABSINTHE_MAIN_TOPLEVEL_WIDTH * (output->geometry.width - 2 * output_gap);
    int32_t width, height;
    width = output->geometry.width - main_toplevel_width - borders_width - layout_gap - 2 * output_gap;
    int32_t dy = output_gap;
    size_t i = 0;
    wl_list_for_each(toplevel, &output->server->toplevels, link) {
        if (toplevel->output != output)
            continue;

        if (i == 0) {
            height = output->geometry.height - borders_width - 2 * output_gap;
            wlr_xdg_toplevel_set_size(toplevel->toplevel.xdg, main_toplevel_width, height);
            toplevel->geometry.x = output->geometry.x + output_gap;
            toplevel->geometry.y = output->geometry.y + output_gap;
        } else {
            height = (output->geometry.height - dy - output_gap) / (toplevels_count - i) - borders_width;
            wlr_xdg_toplevel_set_size(toplevel->toplevel.xdg, width, height);
            toplevel->geometry.x = output->geometry.x + main_toplevel_width + layout_gap + output_gap;
            toplevel->geometry.y = output->geometry.y + dy;
            dy += height + borders_width + layout_gap;
        }

        i++;
    }
}
