
Module['preRun'].push(function() {
    ENV["TAISEI_NOASYNC"] = "1";
    ENV["TAISEI_NOUNLOAD"] = "1";
    ENV["TAISEI_PREFER_SDL_VIDEODRIVERS"] = "emscripten";
    ENV["TAISEI_RENDERER"] = "gles30";

    FS.mkdir('/persistent');
    FS.mount(IDBFS, {}, '/persistent');

    // This function has been removed from Emscripten, but SDL still uses it...
    Module['Pointer_stringify'] = function(ptr) {
        return UTF8ToString(ptr);
    }

    Pointer_stringify = Module['Pointer_stringify']
    window.Pointer_stringify = Module['Pointer_stringify']
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

Module['preinitializedWebGLContext'] = document.getElementById('canvas').getContext('webgl2', {
    alpha : false,
    antialias : false,
    depth : false,
    powerPreference : 'high-performance',
    premultipliedAlpha : true,
    preserveDrawingBuffer : false,
    stencil : false,
});

// Module['preinitializedWebGLContext'] = WebGLDebugUtils.makeDebugContext(Module['preinitializedWebGLContext']);
