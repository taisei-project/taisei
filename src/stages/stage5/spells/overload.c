/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

typedef struct Node Node;

struct Node {
	cmplx pos;
	Node *powered_by;
	BoxedLaser power_laser;
	union {
		struct {
			Node *top;
			Node *left;
			Node *right;
			Node *bottom;
		};
		Node *array[4];
	} links;
};

typedef struct NodesGrid {
	int rows;
	int columns;
	Node *active;
	Node nodes[];
} NodesGrid;

INLINE Node *get_node(NodesGrid *grid, int row, int column) {
	int rows = grid->rows;
	int columns = grid->columns;

	if(row < 0 || row >= rows || column < 0 || column >= columns) {
		return NULL;
	}

	auto nodes = (Node (*)[rows][columns])&grid->nodes;
	return &(*nodes)[row][column];
}

static void setup_nodes_grid(NodesGrid *grid, Rect area) {
	int rows = grid->rows;
	int columns = grid->columns;

	for(int r = 0; r < rows; ++r) {
		real y = lerp(area.top, area.bottom, r / (rows - 1.0));

		for(int c = 0; c < columns; ++c) {
			real x = lerp(area.left, area.right, c / (columns - 1.0));

			Node *n = get_node(grid, r, c);
			n->pos = CMPLX(x, y);
			n->links.top    = get_node(grid, r - 1, c);
			n->links.bottom = get_node(grid, r + 1, c);
			n->links.left   = get_node(grid, r,     c - 1);
			n->links.right  = get_node(grid, r,     c + 1);
		}
	}
}

INLINE bool node_is_active(const NodesGrid *grid, const Node *n) {
	return grid->active == n;
}

INLINE bool node_is_powered(const NodesGrid *grid, const Node *n) {
	return n->powered_by != NULL || node_is_active(grid, n) || ENT_UNBOX(n->power_laser) != NULL;
}

static Node *nearest_link(NodesGrid *grid, Node *n, cmplx pos) {
	Node *ignore = n->powered_by;
	Node *mindist_unpowered_link = NULL;
	Node *mindist_link = NULL;
	real mindist_unpowered = HUGE_VAL;
	real mindist = HUGE_VAL;

	for(int i = 0; i < ARRAY_SIZE(n->links.array); ++i) {
		Node *l = n->links.array[i];

		if(l == ignore || l == NULL) {
			continue;
		}

		real d = cabs2(l->pos - pos);

		if(!node_is_powered(grid, l) && d < mindist_unpowered) {
			mindist_unpowered = d;
			mindist_unpowered_link = l;
		}

		if(d < mindist) {
			mindist = d;
			mindist_link = l;
		}
	}

	return mindist_unpowered_link ?: NOT_NULL(mindist_link);
}

static Node *nearest_node(NodesGrid *grid, cmplx pos) {
	int num_nodes = grid->rows * grid->columns;
	Node *mindist_node = &grid->nodes[0];
	real mindist = cabs2(mindist_node->pos - pos);

	for(int i = 1; i < num_nodes; ++i) {
		Node *n = grid->nodes + i;
		real d = cabs2(n->pos - pos);

		if(d < mindist) {
			mindist = d;
			mindist_node = n;
		}
	}

	return mindist_node;
}

TASK(grid_visual, { NodesGrid *grid; }) {
	auto grid = ARGS.grid;
	int rows = grid->rows;
	int columns = grid->columns;

	Sprite *spr_base = res_sprite("enemy/swirl");
	Sprite *spr_flame = res_sprite("part/smoothdot");
	Sprite *spr_target = res_sprite("fairy_circle_red");

	for(int t = 0;; ++t, YIELD) {
		Node *next = grid->active ? nearest_link(grid, grid->active, global.plr.pos) : NULL;
		for(int r = 0; r < rows; ++r) {
			for(int c = 0; c < columns; ++c) {
				Node *n = NOT_NULL(get_node(grid, r, c));

				PARTICLE(
					.pos = n->pos,
					.color = n == next ? RGBA(2, 0, 0, 0) : RGBA(1, 1, 1, 0),
					.sprite_ptr = spr_base,
					.angle = t * -M_PI/32,
					.flags = PFLAG_MANUALANGLE | PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE,
					.timeout = 30,
					.draw_rule = pdraw_timeout_fade(0.05, 0),
					.layer = LAYER_PARTICLE_LOW,
				);

				if(n == next) {
					PARTICLE(
						.pos = n->pos,
						.color = RGBA(1, 1, 1, 0),
						.sprite_ptr = spr_target,
						.angle = t * M_PI/32,
						.flags = PFLAG_MANUALANGLE | PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE,
						.timeout = 20,
						.draw_rule = pdraw_timeout_scalefade(1.2, 1, 0.1, 0),
						.layer = LAYER_PARTICLE_HIGH,
					);
				}

				if(!node_is_powered(grid, n) || t % 5 != 0) {
					continue;
				}

				cmplx offset = rng_sreal() * 5;
				offset += rng_sreal() * 4i;

				PARTICLE(
					.sprite_ptr = spr_flame,
					.pos = n->pos + offset,
					.color = node_is_active(grid, n) ?
						RGBA(1.0, 0.5, 0.0, 0.0) : RGBA(0.0, 0.5, 0.5, 0.0),
					.draw_rule = pdraw_timeout_scale(2, 0),
					.move = move_linear((-50.0i - offset) / 50.0),
					.timeout = 50,
					.scale = 0.75+I,
				);
			}
		}
	}
}

static void explode_circuit(Node *head) {
	int cnt = difficulty_value(8, 10, 12, 14);
	cmplx r = cdir(M_TAU/cnt);

	for(Node *next; head; head = next) {
		cmplx v = 1.5*I;

		for(int i = 0; i < cnt; ++i, v *= r) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = head->pos,
				.color = RGBA(0, 1, 1, 0),
				.move = move_asymptotic_simple(v, 8),
			);
		}

		next = head->powered_by;
		head->powered_by = NULL;

		Laser *l = ENT_UNBOX(head->power_laser);
		if(l != NULL) {
			// FIXME why does this not work?
			// clear_laser(l, CLEAR_HAZARDS_FORCE | CLEAR_HAZARDS_NOW);
			l->deathtime = global.frames - l->birthtime + 20;
		}

		stage_shake_view(16);
	}

	play_sfx("boom");
}

static void set_active(NodesGrid *grid, Node *n) {
	grid->active = n;

	if(global.diff > D_Easy) {
		int cnt = difficulty_value(2, 5, 7, 10);
		cmplx v = 2 * rng_dir();
		cmplx r = cdir(M_TAU/cnt);

		for(int i = 0; i < cnt; ++i, v *= r) {
			PROJECTILE(
				.proto = pp_rice,
				.pos = n->pos,
				.color = RGBA(1, 1, 0, 0),
				.move = move_asymptotic_simple(v, 2),
			);
		}

		play_sfx("shot2");
	}
}

TASK(grid_logic, { CoEvent *trigger; NodesGrid **grid_out; }) {
	real pad = 16;
	Rect area = {
		.top_left = pad + pad * I, //60i,
		.bottom_right = (VIEWPORT_W - pad) + (VIEWPORT_H - pad) * I
	};
	int spread_time = difficulty_value(50, 45, 40, 35);
	int rows = 8;
	int columns = 7;
	int num_nodes = rows * columns;
	NodesGrid *grid = TASK_MALLOC(sizeof(*grid) + sizeof(Node) * num_nodes);
	grid->rows = rows;
	grid->columns = columns;
	setup_nodes_grid(grid, area);
	*ARGS.grid_out = grid;

	INVOKE_SUBTASK(grid_visual, grid);

	for(;;) {
		while(!grid->active) {
		wait_for_active_node:
			YIELD;
		}

		WAIT(spread_time);

		if(!grid->active) {
			goto wait_for_active_node;
		}

		Node *next = nearest_link(grid, grid->active, global.plr.pos);

		Laser *laser = create_laserline_ab(grid->active->pos, next->pos, 16, 15, 99999, RGBA(0.3, 0.9, 1, 0));
		laser->unclearable = true;
		laser->ent.draw_layer = LAYER_LASER_LOW;
		grid->active->power_laser = ENT_BOX(laser);

		if(node_is_powered(grid, next)) {
			explode_circuit(grid->active);
			grid->active = NULL;
			coevent_signal(ARGS.trigger);
		} else {
			next->powered_by = grid->active;
			set_active(grid, next);
		}
	}
}

TASK(trigger_ball_particles, { BoxedProjectile p; }) {
	auto p = TASK_BIND(ARGS.p);
	Sprite *sprites[2] = {
		res_sprite("part/lightning0"),
		res_sprite("part/lightning1"),
	};

	for(;;YIELD) {
		RNG_ARRAY(rng, 5);
		PARTICLE(
			.sprite_ptr = sprites[vrng_chance(rng[0], 0.5)],
			.pos = p->pos + 3 * (vrng_sreal(rng[1]) + I * vrng_sreal(rng[2])),
			.angle = vrng_angle(rng[3]),
			.color = RGBA(1.0, vrng_range(rng[4], 0.7, 0.9), 0.4, 0.0),
			.timeout = 20,
			.draw_rule = pdraw_timeout_scalefade(0, 3.4, 1, 0),
			.flags = PFLAG_NOMOVE | PFLAG_MANUALANGLE,
		);
	}
}

TASK(trigger_ball, { cmplx pos; NodesGrid *grid; int charge_time; }) {
	int charge_time = ARGS.charge_time;
	int discharge_time = charge_time;
	Node *target = nearest_node(ARGS.grid, global.plr.pos);

	Color charge_color = *RGBA(0.5, 0.2, 1.0, 0.0);

	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_soul,
		.pos = ARGS.pos,
		.color = RGBA(0.2, 0.2, 1.0, 0.0),
		.flags = PFLAG_NOCLEAR | PFLAG_NOMOVE | PFLAG_INDESTRUCTIBLE | PFLAG_MANUALANGLE,
		.move = move_from_towards(ARGS.pos, target->pos, 0.05),
		.angle_delta = M_TAU/7,
	));

	INVOKE_SUBTASK(common_charge,
		.time = charge_time,
		.color = &charge_color,
		.anchor = &p->pos,
		.sound = COMMON_CHARGE_SOUNDS,
	);

	INVOKE_SUBTASK(trigger_ball_particles, ENT_BOX(p));

	for(int t = 0; t < charge_time; ++t, YIELD) {
		p->scale = (1.0f + 1.0if) * (0.1f + 0.9f * (t / (charge_time - 1.0f)));
	}

	p->flags &= ~PFLAG_NOMOVE;
	common_charge(discharge_time, &p->pos, 0, &charge_color);
	set_active(ARGS.grid, target);

	int cnt = difficulty_value(8, 10, 12, 14);
	cmplx v = rng_dir();
	cmplx r = cdir(M_TAU/cnt);

	for(int i = 0; i < cnt; ++i, v *= r) {
		PROJECTILE(
			.proto = pp_bigball,
			.pos = p->pos,
			.color = RGBA(1.0, 0.5, 0.0, 0.0),
			.move = move_asymptotic_simple(1.1 * v, 5),
		);

		PROJECTILE(
			.proto = pp_bigball,
			.pos = p->pos,
			.color = RGBA(0.0, 0.5, 1.0, 0.0),
			.move = move_asymptotic_simple(v, 10),
		);
	}

	stage_shake_view(40);
	aniplayer_hard_switch(&global.boss->ani,"main_mirror",0);
	play_sfx("boom");

	kill_projectile(p);
}

TASK(boss_regen, { BoxedBoss boss; }) {
	auto boss = TASK_BIND(ARGS.boss);
	Attack *a = NOT_NULL(boss->current);
	for(;;YIELD) {
		a->hp = min(a->maxhp, a->hp + 2);
	}
}

DEFINE_EXTERN_TASK(stage5_spell_overload) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + 120i, 0.2);
	BEGIN_BOSS_ATTACK(&ARGS);

	COEVENTS_ARRAY(trigger) events;
	TASK_HOST_EVENTS(events);

	NodesGrid *grid;
	INVOKE_SUBTASK(grid_logic, &events.trigger, &grid);

	int charge_time = 120;
	WAIT(120);

	for(;;) {
		aniplayer_hard_switch(&boss->ani, "charge_begin", 1);
		aniplayer_queue(&boss->ani, "charge_idle", 0);
		INVOKE_TASK(trigger_ball,
			.pos = boss->pos - 60i + 10,
			.charge_time = charge_time,
			.grid = grid,
		);
		WAIT(charge_time);
		aniplayer_hard_switch(&boss->ani, "charge_end", 1);
		aniplayer_queue(&boss->ani, "main", 0);
		boss->in_background = true;
		auto regen_task = cotask_box(INVOKE_SUBTASK(boss_regen, ENT_BOX(boss)));
		WAIT_EVENT_OR_DIE(&events.trigger);
		CANCEL_TASK(regen_task);
		boss->in_background = false;
		WAIT(180);
	}
}
