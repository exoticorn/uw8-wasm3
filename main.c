#include "wasm3/source/wasm3.h"
#include "wasm3/source/m3_env.h"
#include <stdio.h>
#include <malloc.h>

void* loadFile(size_t* sizeOut, const char* filename) {
  FILE* file = fopen(filename, "rb");
  assert(file != NULL);
  fseek(file, 0, SEEK_END);
  size_t size = (size_t)ftell(file);
  rewind(file);

  void* buffer = malloc(size);
  assert(fread(buffer, size, 1, file) == 1);
  fclose(file);
  *sizeOut = size;
  return buffer;
}

void verifyM3(M3Result result) {
  if(result != NULL) {
    fprintf(stderr, "Error: %s\n", result);
    exit(1);
  }
}

int main() {
  IM3Environment env = m3_NewEnvironment();
  IM3Runtime runtime = m3_NewRuntime(env, 16384, NULL);
  runtime->memory.maxPages = 4;
  verifyM3(ResizeMemory(runtime, 4));

  uint8_t* memory = m3_GetMemory(runtime, NULL, 0);
  assert(memory != NULL);

  size_t loaderSize;
  void* loaderWasm = loadFile(&loaderSize, "loader.wasm");

  IM3Module loaderMod;
  verifyM3(m3_ParseModule(env, &loaderMod, loaderWasm, loaderSize));
  loaderMod->memoryImported = true;
  verifyM3(m3_LoadModule(runtime, loaderMod));

  size_t platformUw8Size;
  void* platformUw8 = loadFile(&platformUw8Size, "platform.uw8");
  memcpy(memory, platformUw8, platformUw8Size);
  printf("platform.uw8 size: %u\n", (unsigned int)platformUw8Size);

  IM3Function loadUw8;
  verifyM3(m3_FindFunction(&loadUw8, runtime, "load_uw8"));
  verifyM3(m3_CallV(loadUw8, (uint32_t)platformUw8Size));
  uint32_t platformSize;
  verifyM3(m3_GetResultsV(loadUw8, &platformSize));
  printf("platform size: %u\n", platformSize);
  printf("First byte: %u\n", memory[0]);

  IM3Module platformMod;
  verifyM3(m3_ParseModule(env, &platformMod, memory, platformSize));
  platformMod->memoryImported = true;
  verifyM3(m3_LoadModule(runtime, platformMod));

  return 0;
}
