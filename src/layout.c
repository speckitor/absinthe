#include "config.h"
#include "focus.h"
#include "toplevel.h"
#include "types.h"

void
layout_arrange(struct absinthe_output *output)
{
	if (!output)
		return;

	struct absinthe_toplevel *toplevel;
	size_t toplevels_count = 0;
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

	int32_t output_gap = OUTPUT_GAP;

	if (toplevels_count == 1) {
		wl_list_for_each(toplevel, &output->server->toplevels, link)
		{
			if (toplevel->output == output && toplevel->tiled)
				break;
		}
		int32_t new_x = output->geom.x + output_gap;
		int32_t new_y = output->geom.y + output_gap;
		toplevel_set_size(toplevel, output->geom.width - 2 * output_gap,
		    output->geom.height - 2 * output_gap);
		toplevel_set_pos(toplevel, new_x, new_y);
		return;
	}

	int32_t layout_gap = LAYOUT_GAP;
	int32_t main_stack_width = (toplevels_count <= output->mstack_size) ?
	    output->geom.width - 2 * output_gap :
	    output->mstack_width * (output->geom.width - 2 * output_gap);
	int32_t width = output->geom.width - main_stack_width - 2 * output_gap -
	    layout_gap;
	int32_t height;
	int32_t dy = output_gap;
	size_t i = 0;

	if (toplevels_count <= output->mstack_size) {
		wl_list_for_each(toplevel, &output->server->toplevels, link)
		{
			if (toplevel->output != output || !toplevel->tiled)
				continue;

			height = (output->geom.height - dy - output_gap) /
			    (toplevels_count - i);
			int32_t new_x = output->geom.x + output_gap;
			int32_t new_y = output->geom.y + dy;
			toplevel_set_pos(toplevel, new_x, new_y);
			toplevel_set_size(toplevel, main_stack_width, height);
			dy += height + layout_gap;

			i++;
		}
		return;
	}

	wl_list_for_each(toplevel, &output->server->toplevels, link)
	{
		if (toplevel->output != output || !toplevel->tiled)
			continue;

		if (i < output->mstack_size) {
			height = (output->geom.height - dy - output_gap) /
			    (output->mstack_size - i);
			int32_t new_x = output->geom.x + output_gap;
			int32_t new_y = output->geom.y + dy;
			toplevel_set_pos(toplevel, new_x, new_y);
			toplevel_set_size(toplevel, main_stack_width, height);
			dy += height + layout_gap;
		} else {
			if (i == output->mstack_size)
				dy = output_gap;
			height = (output->geom.height - dy - output_gap) /
			    (toplevels_count - i);
			toplevel->geom.x = output->geom.x + main_stack_width +
			    layout_gap + output_gap;
			toplevel->geom.y = output->geom.y + dy;
			toplevel_set_size(toplevel, width, height);
			dy += height + layout_gap;
		}

		i++;
	}
}
