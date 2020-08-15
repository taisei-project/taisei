



static uint stage5_stairs_pos(Stage3D *s3d, vec3 pos, float maxrange) {
    vec3 p = {0, 0, 0};
    vec3 r = {0, 0, 6000};

    return linear3dpos(s3d, pos, maxrange, p, r);
}

static void stage5_stairs_draw(vec3 pos) {
    r_state_push();
    r_mat_mv_push();
    r_mat_mv_translate(pos[0], pos[1], pos[2]);
    r_mat_mv_scale(300,300,300);
    r_shader("tower_light");
    r_uniform_sampler("tex", "stage5/tower");
    r_uniform_vec3("lightvec", 0, 0, 0);
    r_uniform_vec4("color", 0.1, 0.1, 0.5, 1);
    r_uniform_float("strength", stagedata.light_strength);
    r_draw_model("tower");
    r_mat_mv_pop();
    r_state_pop();
}

static void stage5_draw(void) {
    r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 100, 20000);
    stage3d_draw(&stage_3d_context, 30000, 1, (Stage3DSegment[]) { stage5_stairs_draw, stage5_stairs_pos });
}


void iku_spell_bg(Boss *b, int t) {
    fill_viewport(0, 300, 1, "stage5/spell_bg");

    r_blend(BLEND_MOD);
    fill_viewport(0, t*0.001, 0.7, "stage5/noise");
    r_blend(BLEND_PREMUL_ALPHA);

    r_mat_mv_push();
    r_mat_mv_translate(0, -100, 0);
    fill_viewport(t/100.0, 0, 0.5, "stage5/spell_clouds");
    r_mat_mv_translate(0, 100, 0);
    fill_viewport(t/100.0 * 0.75, 0, 0.6, "stage5/spell_clouds");
    r_mat_mv_translate(0, 100, 0);
    fill_viewport(t/100.0 * 0.5, 0, 0.7, "stage5/spell_clouds");
    r_mat_mv_translate(0, 100, 0);
    fill_viewport(t/100.0 * 0.25, 0, 0.8, "stage5/spell_clouds");
    r_mat_mv_pop();

    float opacity = 0.05 * stagedata.light_strength;
    r_color4(opacity, opacity, opacity, opacity);
    fill_viewport(0, 300, 1, "stage5/spell_lightning");
}

void iku_spell_bg(Boss *b, int t) {
    fill_viewport(0, 300, 1, "stage5/spell_bg");

    r_blend(BLEND_MOD);
    fill_viewport(0, t*0.001, 0.7, "stage5/noise");
    r_blend(BLEND_PREMUL_ALPHA);

    r_mat_mv_push();
    r_mat_mv_translate(0, -100, 0);
    fill_viewport(t/100.0, 0, 0.5, "stage5/spell_clouds");
    r_mat_mv_translate(0, 100, 0);
    fill_viewport(t/100.0 * 0.75, 0, 0.6, "stage5/spell_clouds");
    r_mat_mv_translate(0, 100, 0);
    fill_viewport(t/100.0 * 0.5, 0, 0.7, "stage5/spell_clouds");
    r_mat_mv_translate(0, 100, 0);
    fill_viewport(t/100.0 * 0.25, 0, 0.8, "stage5/spell_clouds");
    r_mat_mv_pop();

    float opacity = 0.05 * stagedata.light_strength;
    r_color4(opacity, opacity, opacity, opacity);
    fill_viewport(0, 300, 1, "stage5/spell_lightning");
}
