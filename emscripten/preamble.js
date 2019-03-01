
Taisei.preRun.push(function() {
    ENV["TAISEI_NOASYNC"] = "1";
    ENV["TAISEI_NOUNLOAD"] = "1";
    ENV["TAISEI_PREFER_SDL_VIDEODRIVERS"] = "emscripten";
    ENV["TAISEI_RENDERER"] = "gles30";

    FS.mkdir('/persistent');
    FS.mount(IDBFS, {}, '/persistent');
});

window['SyncFS'] = function(is_load, ccptr) {
    FS.syncfs(is_load, function(err) {
        Taisei.ccall(
            'vfs_sync_callback',
            null, ["boolean", "string", "number"],
            [is_load, err, ccptr]
        );
    });
};
