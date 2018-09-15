
#pragma once

typedef enum ShaderStage {
	SHADER_STAGE_INVALID,
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_FRAGMENT,
} ShaderStage;

typedef enum ShaderLanguage {
	SHLANG_INVALID,
	SHLANG_GLSL,
	SHLANG_SPIRV,
} ShaderLanguage;

typedef struct ShaderSource ShaderSource;
typedef struct ShaderLangInfo ShaderLangInfo;
