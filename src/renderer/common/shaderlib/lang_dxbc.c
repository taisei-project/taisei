/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_dxbc.h"

#include "shaderlib.h"

#include "log.h"

#if defined(SDL_PLATFORM_WINDOWS)
	#define D3DCOMPILER_DLL "d3dcompiler_47.dll"
#elif defined(SDL_PLATFORM_APPLE)
	#define D3DCOMPILER_DLL "libvkd3d-utils.1.dylib"
#else
	#define D3DCOMPILER_DLL "libvkd3d-utils.so.1"
#endif

#if !defined(SDL_PLATFORM_WINDOWS)

/* __stdcall declaration, largely taken from vkd3d_windows.h */
#ifdef __stdcall
	#undef __stdcall
#endif
#if defined(__x86_64__) || defined(__arm64__)
	#define __stdcall __attribute__((ms_abi))
#else
	#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)) || defined(__APPLE__)
		#define __stdcall __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
	#else
		#define __stdcall __attribute__((__stdcall__))
	#endif
#endif

/* Win32 Type Definitions */
typedef int HRESULT;
typedef const void *LPCVOID;
typedef size_t SIZE_T;
typedef const char *LPCSTR;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef void *REFIID;

#endif  // !defined(SDL_PLATFORM_WINDOWS)

/* ID3DBlob definition, used by both D3DCompiler and DXCompiler */
typedef struct ID3DBlob ID3DBlob;
typedef struct ID3DBlobVtbl
{
    HRESULT(__stdcall *QueryInterface)
    (ID3DBlob *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)
    (ID3DBlob *This);
    ULONG(__stdcall *Release)
    (ID3DBlob *This);
    LPVOID(__stdcall *GetBufferPointer)
    (ID3DBlob *This);
    SIZE_T(__stdcall *GetBufferSize)
    (ID3DBlob *This);
} ID3DBlobVtbl;
struct ID3DBlob
{
    const ID3DBlobVtbl *lpVtbl;
};
#define ID3D10Blob ID3DBlob

/* d3dcompiler Type Definitions */
typedef void D3D_SHADER_MACRO; /* hack, unused */
typedef void ID3DInclude;      /* hack, unused */

typedef HRESULT(__stdcall *pfn_D3DCompile)(
	LPCVOID pSrcData,
	SIZE_T SrcDataSize,
	LPCSTR pSourceName,
	const D3D_SHADER_MACRO *pDefines,
	ID3DInclude *pInclude,
	LPCSTR pEntrypoint,
	LPCSTR pTarget,
	UINT Flags1,
	UINT Flags2,
	ID3DBlob **ppCode,
	ID3DBlob **ppErrorMsgs);

typedef struct DXBCCompiler {
	void *dll;
	pfn_D3DCompile D3DCompile;
} DXBCCompiler;

static DXBCCompiler *compiler;

bool dxbc_init_compiler(void) {
	DXBCCompiler comp = { };
	comp.dll = SDL_LoadObject(D3DCOMPILER_DLL);

	if(!comp.dll) {
		log_sdl_error(LOG_ERROR, "SDL_LoadObject");
		return false;
	}

	comp.D3DCompile = UNION_CAST(SDL_FunctionPointer, pfn_D3DCompile, SDL_LoadFunction(comp.dll, "D3DCompile"));

	if(!comp.D3DCompile) {
		SDL_UnloadObject(comp.dll);
		log_sdl_error(LOG_ERROR, "SDL_LoadFunction");
		return false;
	}

	log_debug(D3DCOMPILER_DLL " loaded");

	compiler = mem_dup(&comp, sizeof(comp));
	return true;
}

void dxbc_shutdown_compiler(void) {
	if(compiler) {
		SDL_UnloadObject(compiler->dll);
		log_debug(D3DCOMPILER_DLL " unloaded");
		mem_free(compiler);
		compiler = NULL;
	}
}

static const char *profile_prefix(ShaderStage stage) {
	switch(stage) {
		case SHADER_STAGE_VERTEX:	return "vs";
		case SHADER_STAGE_FRAGMENT: return "ps";
		default: UNREACHABLE;
	}
}

bool dxbc_compile(const ShaderSource *in, ShaderSource *out, MemArena *arena, const DXBCCompileOptions *options) {
	if(!compiler) {
		log_error("D3DCompiler has not been initialized");
		return false;
	}

	assert(in->lang.lang == SHLANG_HLSL);

	char profile[32];
	snprintf(profile, sizeof(profile), "%s_%u_%u",
		profile_prefix(in->stage),
		in->lang.hlsl.shader_model / 10,
		in->lang.hlsl.shader_model % 10);

	log_debug("profile: %s", profile);

	ID3DBlob *blob = NULL;
    ID3DBlob *error_blob = NULL;

	const char *entrypoint = options->entrypoint ?: in->entrypoint;

    HRESULT ret = compiler->D3DCompile(
		in->content,
		in->content_size - 1,
		NULL,
		NULL,
		NULL,
		entrypoint,
		profile,
		0,
		0,
		&blob,
		&error_blob);

	char *error_str = error_blob ? error_blob->lpVtbl->GetBufferPointer(error_blob) : NULL;

	if(ret < 0) {
		assert(error_str != NULL);
		log_error("Failed to compile HLSL shader: %s", error_str);
		return false;
	}

	if(error_str) {
		log_warn("Non-fatal errors when compiling HLSL shader: %s", error_str);
	}

	assert(blob != NULL);

	auto content = blob->lpVtbl->GetBufferPointer(blob);
	auto content_size = blob->lpVtbl->GetBufferSize(blob);

	*out = (ShaderSource) {
		.lang = {
			.lang = SHLANG_DXBC,
			.dxbc.shader_model = in->lang.hlsl.shader_model,
		},
		.stage = in->stage,
		.content = marena_memdup(arena, content, content_size),
		.content_size = content_size,
		.entrypoint = marena_strdup(arena, entrypoint),
	};

	blob->lpVtbl->Release(blob);
	return true;
}
