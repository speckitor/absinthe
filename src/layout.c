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

    int32_t bw = ABSINTHE_WINDOW_BORDER_WIDTH;
    int32_t og = ABSINTHE_OUTPUT_GAP;

    if (toplevels_count == 1) {
        wl_list_for_each(toplevel, &output->server->toplevels, link) {
            if (toplevel->output == output)
                break;
        }

        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel,
                                  output->geometry.width - 2 * bw - 2 * og,
                                  output->geometry.height - 2 * bw - 2 * og);
        toplevel->geometry.x = output->geometry.x + og;
        toplevel->geometry.y = output->geometry.y + og;
        return;
    }

    int32_t w, h;
    int32_t lg = ABSINTHE_LAYOUT_GAP;
    int32_t mw = ABSINTHE_MAIN_TOPLEVEL_WIDTH * (output->geometry.width - 2 * og);
    int32_t ty = og;
    size_t i = 0;
    wl_list_for_each(toplevel, &output->server->toplevels, link) {
        if (toplevel->output != output)
            continue;

        if (i == 0) {
            h = output->geometry.height - 2 * bw - 2 * og;
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, mw, h);
            toplevel->geometry.x = output->geometry.x + og;
            toplevel->geometry.y = output->geometry.y + og;
        } else {
            w = output->geometry.width - mw - 2 * bw - 2 * lg - 2 * og;
            h = (output->geometry.height - ty - og) / (toplevels_count - i) - 2 * bw;
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, w, h);
            toplevel->geometry.x = output->geometry.x + mw + lg + og;
            toplevel->geometry.y = output->geometry.y + ty;
            ty += h + 2 * bw + lg;
        }

        i++;
    }
}
