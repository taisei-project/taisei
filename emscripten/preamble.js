
Module['totalDependencies'] = 0;
Module['monitorRunDependencies'] = function(left) {
    Module['totalDependencies'] = Math.max(Module['totalDependencies'], left);
    Module['setStatus'](
        left ? 'Preparing… (' + (Module['totalDependencies']-left) + '/' + Module['totalDependencies'] + ')'
             : 'All downloads complete.');
};

Module['initFilesystem'] = function() {
    FS.mkdir('/persistent');
    FS.mount(IDBFS, {}, '/persistent');
};

// WebGL extensions have to be explicitly enabled to make the functionality available.
// Note that Emscripten will always report all *supported* extensions in GL_EXTENSIONS,
// regardless of whether they are enabled or not. This is non-conformant from the GLES
// perspective. The easiest way to fix that is to enable all of them here.
var glContext = Module['preinitializedWebGLContext'];
glContext.getSupportedExtensions().forEach(function(ext) {
    glContext.getExtension(ext);
});

function SyncFS(is_load, ccptr) {
    FS.syncfs(is_load, function(err) {
        Module['ccall'](
            'vfs_sync_callback',
            null, ["boolean", "string", "number"],
            [is_load, err, ccptr],
            { async: true }
        );
    });
}

if(typeof dynCall === 'undefined') {
    dynCall = Module['dynCall'] = function dynCall(sig, ptr, args) {
        return wasmTable.get(ptr).apply(this, args);
    }
};

var debug_tables;  // closure may fail on debug builds without this
