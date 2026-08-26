
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>

#include <wgslcrap.h>

static void *alloc_fn(size_t size, void *arg) {
	return malloc(size);
}

int main(int argc, char **argv) {
	size_t spv_alloc_size = (1 << 14);
	size_t spv_size = 0;
	char *spirv = malloc(spv_alloc_size);
	assert(spirv != NULL);
	FILE *in = stdin;

	while(!feof(in)) {
		size_t avail = spv_alloc_size - spv_size;

		if(avail < 1) {
			spv_alloc_size *= 2;
			spirv = realloc(spirv, spv_alloc_size);
			assert(spirv != NULL);
			avail = spv_alloc_size - spv_size;
		}

		size_t r = fread(spirv + spv_size, 1, avail, in);

		if(r == 0) {
			break;
		}

		spv_size += r;
	}

	assert((spv_size % sizeof(uint32_t)) == 0);

	WGSLResult r = spirv_to_wgsl((uint32_t*)spirv, spv_size / sizeof(uint32_t), (WGSLAllocator) {
		.func = alloc_fn,
	});

	if(r.error) {
		assert(!r.content);
		fprintf(stderr, "%s\n", r.error);
		free(r.error);
		return 1;
	}

	assert(r.content);

	fwrite(r.content, r.content_size, 1, stdout);
	fflush(stdout);

	return 0;
}
