/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "../stagex.h"
#include "stage.h"
#include "global.h"
#include "util/glm.h"

typedef struct LatticeNode LatticeNode;
struct LatticeNode {
	cmplx ofs;
	LatticeNode *neighbours[6];
	BoxedProjectile dot;
	int particle_spawn_time;
	uint32_t visited;
	uint32_t occupancy;
};

typedef struct Infnet {
	LatticeNode *nodes;
	int *idxmap;
	int num_nodes;
	int num_nodes_used;
	cmplx origin;
	real radius;
} Infnet;

static int lattice_index(int sidelen, int x, int y) {
	if(x >= sidelen || y >= sidelen || x < 0 || y < 0) {
		return -1;
	}

	return y * sidelen + x;
}

static void init_lattice_nodes(int sidelen, LatticeNode out_nodes[]) {
	cmplx origin = CMPLX(0.5, 0.5);
	LatticeNode *node = out_nodes;

	for(int i = 0; i < sidelen; ++i) {
		real ifactor = i / (sidelen - 1.0);
		for(int j = 0; j < sidelen; ++j) {
			cmplx ofs = ((j + 0.5 * (i & 1)) / (sidelen - 1.0)) + I * ifactor - origin;

			int nindices[6];

			if(i & 1) {
				nindices[0] = lattice_index(sidelen, j,     i - 1);
				nindices[1] = lattice_index(sidelen, j + 1, i - 1);
				nindices[2] = lattice_index(sidelen, j + 1, i    );
				nindices[3] = lattice_index(sidelen, j + 1, i + 1);
				nindices[4] = lattice_index(sidelen, j    , i + 1);
				nindices[5] = lattice_index(sidelen, j - 1, i    );
			} else {
				nindices[0] = lattice_index(sidelen, j - 1, i - 1);
				nindices[1] = lattice_index(sidelen, j    , i - 1);
				nindices[2] = lattice_index(sidelen, j + 1, i    );
				nindices[3] = lattice_index(sidelen, j,     i + 1);
				nindices[4] = lattice_index(sidelen, j - 1, i + 1);
				nindices[5] = lattice_index(sidelen, j - 1, i    );
			}

			*node = (LatticeNode) {
				.ofs = ofs * 2,
			};

			for(int n = 0; n < 6; ++n) {
				if(nindices[n] >= 0) {
					node->neighbours[n] = &out_nodes[nindices[n]];
				}
			}

			++node;
		}
	}
}

static int idxmap_cmp(const void *p1, const void *p2, void *ctx) {
	LatticeNode *nodes = ctx;
	const int *i1 = p1;
	const int *i2 = p2;

	real d1 = cabs2(nodes[*i1].ofs);
	real d2 = cabs2(nodes[*i2].ofs);

	int c = (d1 > d2) - (d2 > d1);

	if(c == 0) {
		// stabilize sort
		return p1 < p2 ? -1 : 1;
	}

	return c;
}

static void init_idxmap(int num_nodes, int idxmap[num_nodes], LatticeNode nodes[num_nodes]) {
	for(int i = 0; i < num_nodes; ++i) {
		idxmap[i] = i;
	}

	qsort_r(idxmap, num_nodes, sizeof(*idxmap), idxmap_cmp, nodes);
}

#define MAX_RANK 0xffffffff

static uint rank_step(uint id, uint base, int dir, LatticeNode *prev) {
	LatticeNode *n = prev->neighbours[dir];

	if(!n || !ENT_UNBOX(n->dot)) {
		return base == 0xff ? 0 : MAX_RANK;
	}

	uint rank = 0;
	rank <<= 1; rank |= (n->visited == id);
	rank <<= 8; rank |= n->occupancy;

	if(base != 0xff) {
		rank <<= 10;
		rank += rank_step(id, 0xff, dir, n);
		rank += rank_step(id, 0xff, (6 + dir - 1) % 6, n);
		rank += rank_step(id, 0xff, (6 + dir + 1) % 6, n);
		rank += rank_step(id, 0xff, (6 + dir - 2) % 6, n);
		rank += rank_step(id, 0xff, (6 + dir + 2) % 6, n);
		rank <<= 4; rank |= base;
	}

	return rank;
}

static int build_path(Infnet *infnet, int max_steps, int path[max_steps], int start, uint32_t id) {
	path[0] = infnet->idxmap[start];
	LatticeNode *prev_node = &infnet->nodes[path[0]];
	int prev_direction = rng_i32_range(0, 6);
	bool prefer_ccw = rng_bool();

	for(int step = 1; step < max_steps; ++step) {
		prev_node->visited = id;
		prev_node->occupancy++;

		int dir_priority[] = {
			prev_direction,                // continue forward
			(6 + prev_direction + 1) % 6,  // 1 turn clockwise
			(6 + prev_direction - 1) % 6,  // 1 turn counterclockwise
			(6 + prev_direction + 2) % 6,  // 2 turns clockwise
			(6 + prev_direction - 2) % 6,  // 2 turns counterclockwise
			(6 + prev_direction + 3) % 6,  // turn back
		};

		// Randomize left/right turns
		if(rng_chance(0.12)) prefer_ccw = !prefer_ccw;

		if(prefer_ccw) SWAP(dir_priority[1], dir_priority[2]);
		if(prefer_ccw) SWAP(dir_priority[3], dir_priority[4]);

		if(rng_chance(0.75)) {
			// Push forward movement down the priority list
			SWAP(dir_priority[0], dir_priority[1]);
			SWAP(dir_priority[1], dir_priority[2]);
		}

		int step_dir = -1;
		uint step_rank = MAX_RANK;

		// Consider turning straight back on first step only,
		// because the "forward" direction was actually picked randomly there.
		for(int i = 0; i < ARRAY_SIZE(dir_priority) - (step > 1); ++i) {
			int dir = dir_priority[i];
			uint rank = rank_step(id, i, dir, prev_node);

			if(rank < step_rank) {
				step_dir = dir;
				step_rank = rank;
			}
		}

		if(step_dir < 0) {
			return step;
		}

		LatticeNode *next_node = NOT_NULL(prev_node->neighbours[step_dir]);
		assert(ENT_UNBOX(next_node->dot));
		path[step] = next_node - infnet->nodes;
		prev_node = next_node;
		prev_direction = step_dir;
	}

	return max_steps;
}

static void emit_particle(Infnet *infnet, int index, Projectile *dot, Sprite *spr) {
	if(dot->ent.draw_layer == LAYER_NODRAW) {
		return;
	}

	if(global.frames - infnet->nodes[index].particle_spawn_time < 6) {
		return;
	}

	PARTICLE(
		.sprite_ptr = spr,
		.pos = dot->pos,
		.color = color_mul_scalar(COLOR_COPY(&dot->color), 0.75),
		.timeout = 10,
		.draw_rule = pdraw_timeout_scalefade(0.5, 1, 1, 0),
		.angle = rng_angle(),
		.flags = PFLAG_MANUALANGLE,
	);

	infnet->nodes[index].particle_spawn_time = global.frames;
}

TASK(infnet_lasers, { Infnet *infnet; int start_idx; }) {
	Infnet *infnet = ARGS.infnet;
	int max_steps = infnet->num_nodes_used;
	int path[max_steps];
	int path_size = build_path(
		infnet, ARRAY_SIZE(path), path, ARGS.start_idx, THIS_TASK.unique_id);

	BoxedLaser lasers[path_size - 1];
	memset(&lasers, 0, sizeof(lasers));

	int lasers_spawned = 0;
	int lasers_alive = 0;

	Sprite *stardust_spr = res_sprite("part/stardust");

	int t = 0;

	const int laser_charge_time = 120;
	const int laser_duration = 240;

	while(!lasers_spawned || lasers_alive) {
		lasers_alive = 0;

		for(int i = 0; i < path_size - 1; ++i) {
			int index0 = path[i];
			int index1 = path[i + 1];

			LatticeNode *na = infnet->nodes + index0;
			LatticeNode *nb = infnet->nodes + index1;

			Projectile *a = ENT_UNBOX(na->dot);
			Projectile *b = ENT_UNBOX(nb->dot);

			if(a && b) {
				// Bump occupancy if not yet spawned or still present, but not if already dead
				if(!lasers[i].ent || ENT_UNBOX(lasers[i])) {
					na->occupancy++;
					nb->occupancy++;
				}

				if(!lasers[i].ent && t >= 10*i) {
					Laser *l = create_laserline_ab(
						0, 0, 10, laser_charge_time, laser_duration, RGBA(0,0,0,0));
					lasers[i] = ENT_BOX(l);
					++lasers_spawned;
				}

				Laser *l = ENT_UNBOX(lasers[i]);
				if(l) {
					cmplx pa = infnet->radius * na->ofs + infnet->origin;
					cmplx pb = infnet->radius * nb->ofs + infnet->origin;
					laserline_set_ab(l, pa, pb);

					if(l->collision_active) {
						emit_particle(infnet, index0, a, stardust_spr);
						emit_particle(infnet, index1, b, stardust_spr);
					}

					l->color = *RGBA(1.0, 0.1, 0.4, 0);
					color_lerp(&l->color, RGBA(1, 0.4, 0.1, 0), (l->width - 3)/7);
				}
			} else {
				Laser *l = lasers[i].ent ? ENT_UNBOX(lasers[i]) : NULL;
				if(l) {
					clear_laser(l, CLEAR_HAZARDS_FORCE);
				}
			}

			if(ENT_UNBOX(lasers[i])) {
				++lasers_alive;
			}
		}

		++t;
		YIELD;
	}
}

TASK(animate_infnet, { Infnet *infnet; }) {
	Infnet *infnet = ARGS.infnet;

	real intro_time = 300;
	real mid_time = 1500;
	real final_time = 2000;

	real rbase = hypot(VIEWPORT_W, VIEWPORT_H);
	real radius0 = rbase * 8;
	real radius1 = rbase * 4;
	real radius2 = rbase;

	infnet->radius = radius0;

	real min_player_dist = 120;
	cmplx plrvec = infnet->origin - global.plr.pos;

	if(cabs(plrvec) < min_player_dist) {
		infnet->origin = global.plr.pos + min_player_dist * cnormalize(plrvec);
	}

	real t = 0;
	int alive = 0;

	cmplx orig_ofs[infnet->num_nodes_used];
	for(int i = 0; i < ARRAY_SIZE(orig_ofs); ++i) {
		LatticeNode *n = infnet->nodes + infnet->idxmap[i];
		orig_ofs[i] = n->ofs;
	}

	do {
		YIELD;

		real final_factor = glm_ease_sine_out(min(1, t / final_time));
		real intro_factor = glm_ease_back_out(min(1, t / intro_time));

		infnet->radius = lerp(radius1, radius2, final_factor);
		infnet->radius = lerp(radius0, infnet->radius, intro_factor);

		alive = 0;

		for(int j = 0; j < infnet->num_nodes_used; ++j) {
			int i = infnet->idxmap[j];
			LatticeNode *node = &infnet->nodes[i];
			Projectile *dot = ENT_UNBOX(node->dot);

			if(UNLIKELY(!dot)) {
				continue;
			}

			++alive;
			node->occupancy = 0;

			real rot_phase = t / 200.0;
			dot->pos = infnet->radius * node->ofs + infnet->origin;
			dot->prevpos = dot->pos0 = dot->pos;

			real d = glm_ease_sine_inout(min(1, t / mid_time));
			real dphase = (global.frames - dot->birthtime - t) * 0.0025 * d;

			dot->color = *color_lerp(
				RGBA(1, 0, 0, 0), RGBA(0, 1, 0, 0), pcos(rot_phase - 2*dphase));

			cmplx pivot = intro_factor * (0.03 - dphase) * cdir(rot_phase + 0.05);
			node->ofs = orig_ofs[j];
			node->ofs -= pivot;
			node->ofs *= cdir(M_PI * sin(rot_phase));
			node->ofs += pivot;

			if(projectile_in_viewport(dot)) {
				dot->flags &= ~PFLAG_NOCOLLISION;
				dot->ent.draw_layer = LAYER_BULLET;
			} else {
				dot->flags |= PFLAG_NOCOLLISION;
				dot->ent.draw_layer = LAYER_NODRAW;
			}
		};

		capproach_asymptotic_p(&infnet->origin, global.plr.pos, 0.001, 1e-9);
		t += 1;
	} while(alive > 0);
}

DEFINE_EXTERN_TASK(stagex_spell_infinity_network) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->in_background = true;

	int spawntime = 300;
	int sidelen = 37;

	int num_nodes = sidelen * sidelen;
	LatticeNode nodes[sidelen * sidelen];
	init_lattice_nodes(sidelen, nodes);

	int idxmap[num_nodes];
	init_idxmap(num_nodes, idxmap, nodes);

	int num_nodes_used = 0;

	for(int j = 0; j < num_nodes; ++j) {
		int i = idxmap[j];
		if(cabs2(nodes[i].ofs) >= 1) {
			break;
		}
		num_nodes_used++;
	}

	Infnet infnet = {
		.nodes = nodes,
		.num_nodes = num_nodes,
		.num_nodes_used = num_nodes_used,
		.origin = (VIEWPORT_W+VIEWPORT_H*I) * 0.5,
		.idxmap = idxmap,
	};

	INVOKE_SUBTASK(animate_infnet, &infnet);

	for(int j = 0; j < num_nodes_used; ++j) {
		int i = idxmap[j];

		nodes[i].dot = ENT_BOX(PROJECTILE(
			.proto = pp_ball,
			.color = color_lerp(RGBA(1, 0, 0, 0), RGBA(0, 0, 1, 0), j / (num_nodes_used - 1.0)),
			.pos = infnet.origin + infnet.radius * nodes[i].ofs,
			.flags = PFLAG_INDESTRUCTIBLE | PFLAG_NOCLEAR | PFLAG_NOAUTOREMOVE,
		));

		// FIXME: really should be num_nodes_used, but spawntime is tuned for this value already…
		if(j % (1 + num_nodes / spawntime) == 0) YIELD;
	}

	INVOKE_SUBTASK(infnet_lasers, &infnet, 0);

	for(;;) {
		WAIT(60);
		real f = rng_real();
		f *= rng_real();
		f *= rng_real();
		int start = infnet.num_nodes_used * f;
		INVOKE_SUBTASK(infnet_lasers, &infnet, start);
	}
}
