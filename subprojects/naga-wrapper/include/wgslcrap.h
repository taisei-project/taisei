
#pragma once

#include <stdlib.h>
#include <stdint.h>

typedef struct WGSLAllocator {
	void *(*func)(size_t size, void *arg);
	void *arg;
} WGSLAllocator;

typedef struct WGSLResult {
	size_t content_size;
	char *content;
	char *error;
} WGSLResult;

WGSLResult spirv_to_wgsl(const uint32_t *code, uint32_t code_size, WGSLAllocator allocator);
