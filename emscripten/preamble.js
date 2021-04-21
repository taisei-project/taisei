
function E(id) { return document.getElementById(id); }

var statusElement = E('status');
var progressElement = E('progress');
var spinnerElement = E('spinner');
var canvasElement = E('canvas');
var canvasContainerElement = E('canvasContainer');
var logToggleElement = E('logToggle');
var logToggleContainerElement = E('logToggleContainer');
var logContainerElement = E('logContainer');
var logOutputElement = E('output');
var dlMessage = statusElement.innerText;
logToggleElement.checked = false;

window['toggleLog'] = function toggleLog() {
    logContainerElement.hidden = !logToggleElement.checked;
    logOutputElement.scrollTop = logOutputElement.scrollHeight;
}

var glContext = canvasElement.getContext('webgl2', {
    'alpha' : false,
    'antialias' : false,
    'depth' : false,
    'powerPreference' : 'high-performance',
    'premultipliedAlpha' : true,
    'preserveDrawingBuffer' : false,
    'stencil' : false,
});

if(!glContext) {
    throw "Could not create a WebGL 2 context";
}

// glContext = WebGLDebugUtils.makeDebugContext(glContext);

// WebGL extensions have to be explicitly enabled to make the functionality available.
// Note that Emscripten will always report all *supported* extensions in GL_EXTENSIONS,
// regardless of whether they are enabled or not. This is non-conformant from the GLES
// perspective. The easiest way to fix that is to enable all of them here.
glContext.getSupportedExtensions().forEach(function(ext) {
    glContext.getExtension(ext);
});

canvasElement.addEventListener("webglcontextlost", function(e) {
    alert('WebGL context lost. You will need to reload the page.');
    e.preventDefault();
}, false);

logOutputElement.value = ''; // clear browser cache

Module = {
    'preRun': [function() {
        ENV["TAISEI_NOASYNC"] = "1";
        ENV["TAISEI_NOUNLOAD"] = "1";
        ENV["TAISEI_PREFER_SDL_VIDEODRIVERS"] = "emscripten";
        ENV["TAISEI_RENDERER"] = "gles30";

        FS.mkdir('/persistent');
        FS.mount(IDBFS, {}, '/persistent');
    }],
    'postRun': [],
    'onFirstFrame': function() {
        canvasContainerElement.hidden = false;
        logToggleContainerElement.style.display = "inline-block";
        Module['setStatus']('', true);
    },
    'print': function(text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        console.log(text);
        logOutputElement.value += text + "\n";
        logOutputElement.scrollTop = logOutputElement.scrollHeight; // focus on bottom
    },
    'printErr': function(text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
    },
    'canvas': canvasElement,
    'preinitializedWebGLContext': glContext,
    'setStatus': function(text, force) {
        var ss = Module['setStatus'];
        if (!text && !force) return;
        if (!ss.last) ss.last = { time: Date.now(), text: '' };
        if (text === ss.last.text) return;
        var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
        var now = Date.now();
        if (m && now - ss.last.time < 30) return; // if this is a progress update, skip it if too soon
        ss.last.time = now;
        ss.last.text = text;
        if (m) {
            text = m[1];
            progressElement.value = parseInt(m[2])*100;
            progressElement.max = parseInt(m[4])*100;
            progressElement.hidden = false;
            spinnerElement.hidden = false;
        } else {
            progressElement.value = null;
            progressElement.max = null;
            progressElement.hidden = true;
            if (!text) spinnerElement.hidden = true;
        }
        statusElement.innerText = text.replace(/^Downloading(?: data)?\.\.\./, dlMessage).replace('...', '…');
        console.log("[STATUS] " + statusElement.innerText);
    },
    'totalDependencies': 0,
    'monitorRunDependencies': function(left) {
        Module['totalDependencies'] = Math.max(Module['totalDependencies'], left);
        Module['setStatus'](left ? 'Preparing… (' + (Module['totalDependencies']-left) + '/' + Module['totalDependencies'] + ')' : 'All downloads complete.');
    }
};

window.onerror = function(error) {
    Module['setStatus']('Error: ' + error);
};

function SyncFS(is_load, ccptr) {
    FS.syncfs(is_load, function(err) {
        Module['ccall'](
            'vfs_sync_callback',
            null, ["boolean", "string", "number"],
            [is_load, err, ccptr]
        );
    });
}

(function() {
    // Try to enable audio playback as soon as possible.
    // It must happen inside an input event handler.
    // https://github.com/emscripten-core/emscripten/issues/6511
    // https://github.com/emscripten-ports/SDL2/issues/57

    function resumeAudio() {
        var sdl2 = Module['SDL2'];

        if(typeof sdl2 === 'undefined') {
            return;
        }

        if(sdl2.audioContext.state == 'suspended') {
            sdl2.audioContext.resume();
        }

        if(sdl2.audioContext.state == 'running') {
            canvasElement.removeEventListener('click', resumeAudio);
            document.removeEventListener('keydown', resumeAudio);
        }
    }

    canvasElement.addEventListener('click', resumeAudio);
    document.addEventListener('keydown', resumeAudio);
})();

var dynCall;

if(typeof dynCall === 'undefined') {
    dynCall = window['dynCall'] = Module['dynCall'] = function dynCall(sig, ptr, args) {
        return wasmTable.get(ptr).apply(this, args);
    }
};

var debug_tables;  // closure may fail on debug builds without this
