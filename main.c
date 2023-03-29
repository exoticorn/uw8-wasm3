#include "wasm3/source/wasm3.h"
#include "wasm3/source/m3_env.h"
#include <math.h>
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

void verifyM3(IM3Runtime runtime, M3Result result) {
  if (result != m3Err_none) {
    M3ErrorInfo info;
    m3_GetErrorInfo(runtime, &info);
    fprintf(stderr, "WASM error: %s (%s)\n", result, info.message);
    exit(1);
  }
}

m3ApiRawFunction(math1) {
  m3ApiReturnType(float);
  m3ApiGetArg(float, v);
  *raw_return = ((float(*)(float))_ctx->userdata)(v);
  m3ApiSuccess();
}

m3ApiRawFunction(math2) {
  m3ApiReturnType(float);
  m3ApiGetArg(float, a);
  m3ApiGetArg(float, b);
  *raw_return = ((float(*)(float, float))_ctx->userdata)(a, b);
  m3ApiSuccess();
}

m3ApiRawFunction(nopFunc) {
  m3ApiSuccess();
}

void linkSystemFunctions(IM3Module mod) {
  m3_LinkRawFunctionEx(mod, "env", "acos", "f(f)", math1, acosf);
  m3_LinkRawFunctionEx(mod, "env", "asin", "f(f)", math1, asinf);
  m3_LinkRawFunctionEx(mod, "env", "atan", "f(f)", math1, atanf);
  m3_LinkRawFunctionEx(mod, "env", "atan2", "f(ff)", math2, atan2f);
  m3_LinkRawFunctionEx(mod, "env", "cos", "f(f)", math1, cosf);
  m3_LinkRawFunctionEx(mod, "env", "exp", "f(f)", math1, expf);
  m3_LinkRawFunctionEx(mod, "env", "log", "f(f)", math1, logf);
  m3_LinkRawFunctionEx(mod, "env", "sin", "f(f)", math1, sinf);
  m3_LinkRawFunctionEx(mod, "env", "tan", "f(f)", math1, tanf);
  m3_LinkRawFunctionEx(mod, "env", "pow", "f(ff)", math2, powf);

  m3_LinkRawFunction(mod, "env", "logChar", "v(i)", nopFunc);

  for(int i = 9; i < 64; ++i) {
    char name[128];
    sprintf(name, "reserved%d", i);
    m3_LinkRawFunction(mod, "env", name, "v()", nopFunc);
  }
}

void* loadUw8(uint32_t* sizeOut, IM3Runtime runtime, IM3Function loadFunc, uint8_t* memory, const char* filename) {
  size_t uw8Size;
  void* uw8 = loadFile(&uw8Size, filename);
  memcpy(memory, uw8, uw8Size);
  verifyM3(runtime, m3_CallV(loadFunc, (uint32_t)uw8Size));
  verifyM3(runtime, m3_GetResultsV(loadFunc, sizeOut));
  void* wasm = malloc(*sizeOut);
  memcpy(wasm, memory, *sizeOut);
  return wasm;
}

int main() {
  IM3Environment env = m3_NewEnvironment();
  IM3Runtime runtime = m3_NewRuntime(env, 16384, NULL);
  runtime->memory.maxPages = 4;
  verifyM3(runtime, ResizeMemory(runtime, 4));

  uint8_t* memory = m3_GetMemory(runtime, NULL, 0);
  assert(memory != NULL);

  size_t loaderSize;
  void* loaderWasm = loadFile(&loaderSize, "loader.wasm");

  IM3Module loaderMod;
  verifyM3(runtime, m3_ParseModule(env, &loaderMod, loaderWasm, loaderSize));
  loaderMod->memoryImported = true;
  verifyM3(runtime, m3_LoadModule(runtime, loaderMod));
  verifyM3(runtime, m3_CompileModule(loaderMod));
  verifyM3(runtime, m3_RunStart(loaderMod));

  IM3Function loadFunc;
  verifyM3(runtime, m3_FindFunction(&loadFunc, runtime, "load_uw8"));

  uint32_t platformSize;
  void* platformWasm = loadUw8(&platformSize, runtime, loadFunc, memory, "platform.uw8");

  uint32_t cartSize;
  void* cartWasm = loadUw8(&cartSize, runtime, loadFunc, memory, "never_sleeps.uw8");

  IM3Module platformMod;
  verifyM3(runtime, m3_ParseModule(env, &platformMod, platformWasm, platformSize));
  platformMod->memoryImported = true;
  verifyM3(runtime, m3_LoadModule(runtime, platformMod));
  linkSystemFunctions(platformMod);
  verifyM3(runtime, m3_CompileModule(platformMod));
  verifyM3(runtime, m3_RunStart(platformMod));

  IM3Module cartMod;
  verifyM3(runtime, m3_ParseModule(env, &cartMod, cartWasm, cartSize));
  platformMod->memoryImported = true;
  verifyM3(runtime, m3_LoadModule(runtime, cartMod));
  linkSystemFunctions(cartMod);
  verifyM3(runtime, m3_CompileModule(cartMod));
  verifyM3(runtime, m3_RunStart(cartMod));

  return 0;
}
