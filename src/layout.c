#include <wlr/util/log.h>

#include "config.h"
#include "output.h"
#include "toplevel.h"
#include "types.h"

/* returns number of toplevels on workspace */
static int prepare_output(struct absn_output *output)
{
    int res = 0;
    absn_toplevel *toplevel;
    wl_list_for_each(toplevel, &output->server->toplevels, link)
    {
        if (toplevel->workspace == output->workspace) {
            wlr_scene_node_set_enabled(&toplevel->scene_tree->node, true);
            if (!toplevel->floating && !toplevel->fullscreen)
                res++;
        } else if (toplevel->output == output && toplevel->workspace != output->workspace) {
            wlr_scene_node_set_enabled(&toplevel->scene_tree->node, false);
        }
    }

    return res;
}

static void tile(struct absn_output *output)
{
    int toplevels_count = prepare_output(output);

    if (toplevels_count < 1)
        return;

    int32_t og = OUTPUT_GAP;
    struct wlr_box new_geom;

    absn_toplevel *toplevel;
    if (toplevels_count == 1) {
        wl_list_for_each(toplevel, &output->server->toplevels, link)
        {
            if (toplevel->workspace == output->workspace && !toplevel->floating && !toplevel->fullscreen)
                break;
        }
        new_geom.x = output->usable_area.x + og, new_geom.y = output->usable_area.y + og,
        new_geom.width = output->usable_area.width - 2 * og, new_geom.height = output->usable_area.height - 2 * og,

        toplevel_set_geom(toplevel, &new_geom);
        return;
    }

    int32_t lg = LAYOUT_GAP;
    int32_t total_lg;

    int mcount = output->workspace->count;
    float msize = output->workspace->size;

    int32_t main_stack_width =
        (toplevels_count <= mcount) ? output->usable_area.width - 2 * og : msize * (output->usable_area.width - 2 * og);
    int32_t w = output->usable_area.width - main_stack_width - 2 * og - lg;

    int32_t pure_h;
    int32_t h;
    int32_t cur_h;
    int32_t r;

    int32_t dy = og;
    int i = 0;

    if (toplevels_count <= mcount) {
        total_lg = (toplevels_count - 1) * lg;
        pure_h = output->usable_area.height - 2 * og - total_lg;
        h = pure_h / toplevels_count;
        r = pure_h % toplevels_count;

        wl_list_for_each(toplevel, &output->server->toplevels, link)
        {
            if (toplevel->workspace != output->workspace || toplevel->floating || toplevel->fullscreen)
                continue;

            cur_h = h + (i < r ? 1 : 0);

            new_geom.x = output->usable_area.x + og;
            new_geom.y = output->usable_area.y + dy;
            new_geom.width = main_stack_width;
            new_geom.height = cur_h;

            toplevel_set_geom(toplevel, &new_geom);

            dy += cur_h + lg;

            i++;
        }
        return;
    }

    wl_list_for_each(toplevel, &output->server->toplevels, link)
    {
        if (toplevel->workspace != output->workspace || toplevel->floating || toplevel->fullscreen)
            continue;

        if (i < mcount) {
            total_lg = (mcount - 1) * lg;
            pure_h = output->usable_area.height - 2 * og - total_lg;
            h = pure_h / mcount;
            r = pure_h % mcount;

            cur_h = h + (i < r ? 1 : 0);

            new_geom.x = output->usable_area.x + og;
            new_geom.y = output->usable_area.y + dy;
            new_geom.width = main_stack_width;
            new_geom.height = cur_h;

            toplevel_set_geom(toplevel, &new_geom);

            dy += cur_h + lg;
        } else {
            if (i == mcount)
                dy = og;

            int32_t stack_count = toplevels_count - mcount;
            total_lg = (stack_count - 1) * lg;
            pure_h = output->usable_area.height - 2 * og - total_lg;
            h = pure_h / stack_count;
            r = pure_h % stack_count;

            cur_h = h + (i - mcount < r ? 1 : 0);

            new_geom.x = output->usable_area.x + main_stack_width + lg + og;
            new_geom.y = output->usable_area.y + dy;
            new_geom.width = w;
            new_geom.height = cur_h;

            toplevel_set_geom(toplevel, &new_geom);

            dy += cur_h + lg;
        }

        i++;
    }
}

static void tile_left(struct absn_output *output)
{
    int toplevels_count = prepare_output(output);

    if (toplevels_count < 1)
        return;

    int32_t og = OUTPUT_GAP;
    struct wlr_box new_geom;

    absn_toplevel *toplevel;
    if (toplevels_count == 1) {
        wl_list_for_each(toplevel, &output->server->toplevels, link)
        {
            if (toplevel->workspace == output->workspace && !toplevel->floating && !toplevel->fullscreen)
                break;
        }
        new_geom.x = output->usable_area.x + og, new_geom.y = output->usable_area.y + og,
        new_geom.width = output->usable_area.width - 2 * og, new_geom.height = output->usable_area.height - 2 * og,

        toplevel_set_geom(toplevel, &new_geom);
        return;
    }

    int32_t lg = LAYOUT_GAP;
    int32_t total_lg;

    int mcount = output->workspace->count;
    float msize = output->workspace->size;

    int32_t main_stack_width =
        (toplevels_count <= mcount) ? output->usable_area.width - 2 * og : msize * (output->usable_area.width - 2 * og);
    int32_t w = output->usable_area.width - main_stack_width - 2 * og - lg;

    int32_t pure_h;
    int32_t h;
    int32_t cur_h;
    int32_t r;

    int32_t dy = og;
    int i = 0;

    if (toplevels_count <= mcount) {
        total_lg = (toplevels_count - 1) * lg;
        pure_h = output->usable_area.height - 2 * og - total_lg;
        h = pure_h / toplevels_count;
        r = pure_h % toplevels_count;

        wl_list_for_each(toplevel, &output->server->toplevels, link)
        {
            if (toplevel->workspace != output->workspace || toplevel->floating || toplevel->fullscreen)
                continue;

            cur_h = h + (i < r ? 1 : 0);

            new_geom.x = output->usable_area.x + og;
            new_geom.y = output->usable_area.y + dy;
            new_geom.width = main_stack_width;
            new_geom.height = cur_h;

            toplevel_set_geom(toplevel, &new_geom);

            dy += cur_h + lg;

            i++;
        }
        return;
    }

    wl_list_for_each(toplevel, &output->server->toplevels, link)
    {
        if (toplevel->workspace != output->workspace || toplevel->floating || toplevel->fullscreen)
            continue;

        if (i < mcount) {
            total_lg = (mcount - 1) * lg;
            pure_h = output->usable_area.height - 2 * og - total_lg;
            h = pure_h / mcount;
            r = pure_h % mcount;

            cur_h = h + (i < r ? 1 : 0);

            new_geom.x = output->usable_area.x + output->usable_area.width - og - main_stack_width;
            new_geom.y = output->usable_area.y + dy;
            new_geom.width = main_stack_width;
            new_geom.height = cur_h;

            toplevel_set_geom(toplevel, &new_geom);

            dy += cur_h + lg;
        } else {
            if (i == mcount)
                dy = og;

            int32_t stack_count = toplevels_count - mcount;
            total_lg = (stack_count - 1) * lg;
            pure_h = output->usable_area.height - 2 * og - total_lg;
            h = pure_h / stack_count;
            r = pure_h % stack_count;

            cur_h = h + (i - mcount < r ? 1 : 0);

            new_geom.x = output->usable_area.x + og;
            new_geom.y = output->usable_area.y + dy;
            new_geom.width = w;
            new_geom.height = cur_h;

            toplevel_set_geom(toplevel, &new_geom);

            dy += cur_h + lg;
        }

        i++;
    }
}

static void monocle(struct absn_output *output)
{
    int toplevels_count = prepare_output(output);

    if (toplevels_count < 1)
        return;

    int32_t og = OUTPUT_GAP;

    struct wlr_box new_geom = {
        .x = output->usable_area.x + og,
        .y = output->usable_area.y + og,
        .width = output->usable_area.width - 2 * og,
        .height = output->usable_area.height - 2 * og,
    };

    absn_toplevel *toplevel;
    wl_list_for_each(toplevel, &output->server->toplevels, link)
    {
        if (toplevel->workspace != output->workspace || toplevel->floating || toplevel->fullscreen)
            continue;

        toplevel_set_geom(toplevel, &new_geom);
    }
}

void layout_arrange(struct absn_output *output)
{
    if (!output)
        return;

    switch (output->workspace->layout) {
    case LAYOUT_TILE:
        tile(output);
        break;
    case LAYOUT_TILELEFT:
        tile_left(output);
        break;
    case LAYOUT_MONOCLE:
        monocle(output);
        break;
    default: /* currently not implemented layouts */
        tile(output);
        break;
    }
}
