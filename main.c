#include "wasm3/source/wasm3.h"
#include "wasm3/source/m3_env.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_video.h"
#include "SDL2/SDL_render.h"
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

void linkSystemFunctions(IM3Runtime runtime, IM3Module mod) {
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

m3ApiRawFunction(platformTrampoline) {
  IM3Function func = (IM3Function)_ctx->userdata;
  uint32_t retCount = m3_GetRetCount(func);
  uint32_t argCount = m3_GetArgCount(func);
  const void* args[16];
  for(uint32_t i = 0; i < argCount; ++i) {
    args[i] = &_sp[retCount + i];
  }
  verifyM3(runtime, m3_Call(func, m3_GetArgCount(func), args));
  for(uint32_t i = 0; i < retCount; ++i) {
    args[i] = &_sp[i];
  }
  verifyM3(runtime, m3_GetResults(func, retCount, args));
  m3ApiSuccess();
}

void appendType(char* signature, M3ValueType type) {
  if(type == c_m3Type_i32) {
    strcat(signature, "i");
  } else if(type == c_m3Type_i64) {
    strcat(signature, "l");
  } else if(type == c_m3Type_f32) {
    strcat(signature, "f");
  } else {
    fprintf(stderr, "Unsupported platform type %d\n", type);
    exit(1);
  }
}

void linkPlatformFunctions(IM3Runtime runtime, IM3Module cartMod, IM3Module platformMod) {
  for(u32 functionIndex = 0; functionIndex < platformMod->numFunctions; ++functionIndex) {
    M3Function function = platformMod->functions[functionIndex];
    if(function.export_name != NULL) {
      IM3Function iFunc;
      verifyM3(runtime, m3_FindFunction(&iFunc, runtime, function.export_name));
      char signature[128] = { 0 };
      if(m3_GetRetCount(iFunc) > 0) {
        appendType(signature, m3_GetRetType(iFunc, 0));
      } else {
        strcat(signature, "v");
      }
      strcat(signature, "(");
      for(uint32_t i = 0; i < m3_GetArgCount(iFunc); ++i) {
        appendType(signature, m3_GetArgType(iFunc, i));
      }
      strcat(signature, ")");
      m3_LinkRawFunctionEx(cartMod, "env", function.export_name, signature, platformTrampoline, iFunc);
    }
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
  linkSystemFunctions(runtime, platformMod);
  verifyM3(runtime, m3_CompileModule(platformMod));
  verifyM3(runtime, m3_RunStart(platformMod));

  IM3Module cartMod;
  verifyM3(runtime, m3_ParseModule(env, &cartMod, cartWasm, cartSize));
  platformMod->memoryImported = true;
  verifyM3(runtime, m3_LoadModule(runtime, cartMod));
  linkSystemFunctions(runtime, cartMod);
  linkPlatformFunctions(runtime, cartMod, platformMod);
  verifyM3(runtime, m3_CompileModule(cartMod));
  verifyM3(runtime, m3_RunStart(cartMod));

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_CreateWindowAndRenderer(320, 240, SDL_WINDOW_RESIZABLE, &window, &renderer);
  SDL_RenderSetLogicalSize(renderer, 320, 240);
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);

  uint32_t* pixels32 = malloc(320*240*4);

  for(uint32_t time = 0;; time += 16) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      switch(event.type) {
      case SDL_QUIT:
        exit(0);
      }
    }

    *(uint32_t*)(memory + 64) = time;
    
    IM3Function updFunc;
    verifyM3(runtime, m3_FindFunction(&updFunc, runtime, "upd"));
    verifyM3(runtime, m3_CallV(updFunc));

    uint32_t* palette = (uint32_t*)(memory + 0x13000);
    uint8_t* pixels = memory + 120;
    for(uint32_t i = 0; i < 320*240; ++i) {
      pixels32[i] = palette[pixels[i]];
    }
    SDL_UpdateTexture(texture, NULL, pixels32, 320*4);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  return 0;
}
