#include <wlr/types/wlr_layer_shell_v1.h>

#include "layout.h"
#include "types.h"

void layer_arrange(absn_output *output)
{
    struct wlr_box usable_area = output->geom;
    absn_layer_surface *layer_surface;

    wl_list_for_each(layer_surface, &output->layer_surfaces, link)
    {
        if (!layer_surface->wlr->initialized)
            continue;

        wlr_scene_layer_surface_v1_configure(layer_surface->scene_layer, &output->geom, &usable_area);
    }

    output->usable_area = usable_area;
    layout_arrange(output);
}
