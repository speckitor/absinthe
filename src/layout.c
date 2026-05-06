#include "config.h"
#include "toplevel.h"
#include "types.h"

void
layout_arrange(struct absn_output *output)
{
	if (!output)
		return;

	struct absn_toplevel *toplevel;
	int toplevels_count = 0;
	wl_list_for_each(toplevel, &output->server->toplevels, link)
	{
		if (toplevel->output == output && toplevel->tiled) {
			toplevels_count++;
			wlr_scene_node_set_enabled(&toplevel->scene_tree->node,
			    true);
		}
	}

	if (toplevels_count < 1)
		return;

	int32_t og = OUTPUT_GAP;
	struct wlr_box new_geom;

	if (toplevels_count == 1) {
		wl_list_for_each(toplevel, &output->server->toplevels, link)
		{
			if (toplevel->output == output && toplevel->tiled)
				break;
		}
		new_geom.x = output->geom.x + og,
		new_geom.y = output->geom.y + og,
		new_geom.width = output->geom.width - 2 * og,
		new_geom.height = output->geom.height - 2 * og,

		toplevel_set_geom(toplevel, &new_geom);
		return;
	}

	int32_t lg = LAYOUT_GAP;
	int32_t main_stack_width = (toplevels_count <= output->mstack_count) ?
	    output->geom.width - 2 * og :
	    output->mstack_width * (output->geom.width - 2 * og);
	int32_t w = output->geom.width - main_stack_width - 2 * og - lg;
	int32_t h;
	int32_t dy = og;
	int i = 0;

	if (toplevels_count <= output->mstack_count) {
		wl_list_for_each(toplevel, &output->server->toplevels, link)
		{
			if (toplevel->output != output || !toplevel->tiled)
				continue;

			h = (output->geom.height - dy - og) /
			    (toplevels_count - i);

			new_geom.x = output->geom.x + og;
			new_geom.y = output->geom.y + dy;
			new_geom.width = main_stack_width;
			new_geom.height = h;

			toplevel_set_geom(toplevel, &new_geom);

			dy += h + lg;

			i++;
		}
		return;
	}

	wl_list_for_each(toplevel, &output->server->toplevels, link)
	{
		if (toplevel->output != output || !toplevel->tiled)
			continue;

		if (i < output->mstack_count) {
			h = (output->geom.height - dy - og) /
			    (output->mstack_count - i);

			new_geom.x = output->geom.x + og;
			new_geom.y = output->geom.y + dy;
			new_geom.width = main_stack_width;
			new_geom.height = h;

			toplevel_set_geom(toplevel, &new_geom);

			dy += h + lg;
		} else {
			if (i == output->mstack_count)
				dy = og;

			h = (output->geom.height - dy - og) /
			    (toplevels_count - i);

			new_geom.x = output->geom.x + main_stack_width + lg +
			    og;
			new_geom.y = output->geom.y + dy;
			new_geom.width = w;
			new_geom.height = h;

			toplevel_set_geom(toplevel, &new_geom);

			dy += h + lg;
		}

		i++;
	}
}
