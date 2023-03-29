#include "wasm3/source/wasm3.h"
#include "wasm3/source/m3_env.h"
#include <stdio.h>
#include <malloc.h>

void* loadFile(size_t* sizeOut, const char* filename) {
  FILE* file = fopen(filename, "rb");
  fseek(file, 0, SEEK_END);
  size_t size = (size_t)ftell(file);
  rewind(file);

  void* buffer = malloc(size);
  fread(buffer, size, 1, file);
  fclose(file);
  *sizeOut = size;
  return buffer;
}

int main() {
  IM3Environment env = m3_NewEnvironment();
  IM3Runtime runtime = m3_NewRuntime(env, 16384, NULL);
  runtime->memory.maxPages = 4;
  ResizeMemory(runtime, 4);

  uint8_t* memory = m3_GetMemory(runtime, NULL, 0);

  size_t loaderSize;
  void* loaderWasm = loadFile(&loaderSize, "loader.wasm");

  IM3Module loaderMod;
  m3_ParseModule(env, &loaderMod, loaderWasm, loaderSize);
  loaderMod->memoryImported = true;
  m3_LoadModule(runtime, loaderMod);

  size_t runtimeUw8Size;
  void* runtimeUw8 = loadFile(&runtimeUw8Size, "platform.uw8");
  memcpy(memory, runtimeUw8, runtimeUw8Size);

  IM3Function loadUw8;
  m3_FindFunction(&loadUw8, runtime, "load_uw8");
  m3_CallV(loadUw8, (uint32_t)runtimeUw8Size);
  uint32_t runtimeSize;
  m3_GetResultsV(loadUw8, &runtimeSize);
  printf("size: %u\n", runtimeSize);

  return 0;
}
