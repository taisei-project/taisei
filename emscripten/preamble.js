
Module['preRun'].push(function() {
    ENV["TAISEI_NOASYNC"] = "1";
    ENV["TAISEI_NOUNLOAD"] = "1";
    ENV["TAISEI_PREFER_SDL_VIDEODRIVERS"] = "emscripten";
    ENV["TAISEI_RENDERER"] = "gles30";

    FS.mkdir('/persistent');
    FS.mount(IDBFS, {}, '/persistent');
});

function SyncFS(is_load, ccptr) {
    FS.syncfs(is_load, function(err) {
        Module['ccall'](
            'vfs_sync_callback',
            null, ["boolean", "string", "number"],
            [is_load, err, ccptr]
        );
    });
};

Module['preinitializedWebGLContext'] = (function() {
    var glctx = document.getElementById('canvas').getContext('webgl2', {
        alpha : false,
        antialias : false,
        depth : false,
        powerPreference : 'high-performance',
        premultipliedAlpha : true,
        preserveDrawingBuffer : false,
        stencil : false,
    });

    // This actually enables the extension. Galaxy-brain API right here.
    glctx.getExtension('WEBGL_debug_renderer_info');

    // glctx = WebGLDebugUtils.makeDebugContext(glctx);
    return glctx;
})();
