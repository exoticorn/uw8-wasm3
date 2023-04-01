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

void* loadUw8(uint32_t* sizeOut, IM3Runtime runtime, IM3Function loadFunc, const char* filename) {
  size_t uw8Size;
  void* uw8 = loadFile(&uw8Size, filename);
  uint8_t* memory = m3_GetMemory(runtime, NULL, 0);
  memcpy(memory, uw8, uw8Size);
  verifyM3(runtime, m3_CallV(loadFunc, (uint32_t)uw8Size));
  verifyM3(runtime, m3_GetResultsV(loadFunc, sizeOut));
  void* wasm = malloc(*sizeOut);
  memcpy(wasm, memory, *sizeOut);
  return wasm;
}

const uint32_t uw8buttonScanCodes[] = {
  SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
  SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_A, SDL_SCANCODE_S
};

typedef struct {
  IM3Runtime runtime;
  IM3Module platform;
  IM3Module cart;
} Uw8Runtime;

void initRuntime(Uw8Runtime* runtime, IM3Environment env,
                 void* platform, size_t platformSize, void* cart, size_t cartSize) {
  runtime->runtime = m3_NewRuntime(env, 65536, NULL);
  runtime->runtime->memory.maxPages = 4;
  verifyM3(runtime->runtime, ResizeMemory(runtime->runtime, 4));

  verifyM3(runtime->runtime, m3_ParseModule(env, &runtime->platform, platform, platformSize));
  runtime->platform->memoryImported = true;
  verifyM3(runtime->runtime, m3_LoadModule(runtime->runtime, runtime->platform));
  linkSystemFunctions(runtime->runtime, runtime->platform);
  verifyM3(runtime->runtime, m3_CompileModule(runtime->platform));
  verifyM3(runtime->runtime, m3_RunStart(runtime->platform));

  verifyM3(runtime->runtime, m3_ParseModule(env, &runtime->cart, cart, cartSize));
  runtime->platform->memoryImported = true;
  verifyM3(runtime->runtime, m3_LoadModule(runtime->runtime, runtime->cart));
  linkSystemFunctions(runtime->runtime, runtime->cart);
  linkPlatformFunctions(runtime->runtime, runtime->cart, runtime->platform);
  verifyM3(runtime->runtime, m3_CompileModule(runtime->cart));
  verifyM3(runtime->runtime, m3_RunStart(runtime->cart));
}

typedef struct AudioState {
  Uw8Runtime runtime;
  uint8_t* memory;
  IM3Function snd;
  uint8_t registers[32];
  uint32_t sampleIndex;
} AudioState;

void audioCallback(void* userdata, Uint8* stream, int len) {
  AudioState* state = (AudioState*)userdata;
  float* samples = (float*)stream;
  int numSamples = len / sizeof(float);
  memcpy(state->memory + 0x50, state->registers, 32);
  for(int i = 0; i < numSamples; ++i) {
    m3_CallV(state->snd, state->sampleIndex++);
    m3_GetResultsV(state->snd, samples++);
  }
}

int main(int argc, const char** argv) {
  if(argc != 2) {
    fprintf(stderr, "Usage: uw8-wasm3 <UW8-MODULE>\n");
    return 1;
  }

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer);
  SDL_RenderSetLogicalSize(renderer, 320, 240);
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);

  uint32_t* pixels32 = malloc(320*240*4);

  IM3Environment env = m3_NewEnvironment();
  IM3Runtime loaderRuntime = m3_NewRuntime(env, 65536, NULL);
  loaderRuntime->memory.maxPages = 4;
  verifyM3(loaderRuntime, ResizeMemory(loaderRuntime, 4));

  size_t loaderSize;
  void* loaderWasm = loadFile(&loaderSize, "loader.wasm");

  IM3Module loaderMod;
  verifyM3(loaderRuntime, m3_ParseModule(env, &loaderMod, loaderWasm, loaderSize));
  loaderMod->memoryImported = true;
  verifyM3(loaderRuntime, m3_LoadModule(loaderRuntime, loaderMod));
  verifyM3(loaderRuntime, m3_CompileModule(loaderMod));
  verifyM3(loaderRuntime, m3_RunStart(loaderMod));

  IM3Function loadFunc;
  verifyM3(loaderRuntime, m3_FindFunction(&loadFunc, loaderRuntime, "load_uw8"));

  uint32_t platformSize;
  void* platformWasm = loadUw8(&platformSize, loaderRuntime, loadFunc, "platform.uw8");

  uint32_t cartSize;
  void* cartWasm = loadUw8(&cartSize, loaderRuntime, loadFunc, argv[1]);

  m3_FreeRuntime(loaderRuntime);

  bool quit = false;
  while(!quit) {
    Uw8Runtime runtime;
    initRuntime(&runtime, env, platformWasm, platformSize, cartWasm, cartSize);

    uint8_t* memory = m3_GetMemory(runtime.runtime, NULL, 0);
    assert(memory != NULL);

    IM3Function updFunc;
    bool hasUpdFunc = m3_FindFunction(&updFunc, runtime.runtime, "upd") == NULL;

    AudioState audioState;
    initRuntime(&audioState.runtime, env, platformWasm, platformSize, cartWasm, cartSize);
    audioState.memory = m3_GetMemory(audioState.runtime.runtime, NULL, 0);
    if(m3_FindFunction(&audioState.snd, audioState.runtime.runtime, "snd") != NULL) {
      verifyM3(audioState.runtime.runtime, m3_FindFunction(&audioState.snd, audioState.runtime.runtime, "sndGes"));
    }
    memcpy(audioState.registers, audioState.memory + 0x50, 32);
    audioState.sampleIndex = 0;

    SDL_AudioSpec audioSpec;
    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_F32SYS;
    audioSpec.channels = 2;
    audioSpec.samples = 256;
    audioSpec.callback = audioCallback;
    audioSpec.userdata = &audioState;
    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &audioSpec, &audioSpec, 0);
    SDL_PauseAudioDevice(audioDevice, 0);

    uint32_t startTime = SDL_GetTicks();

    bool restart = false;
    while(!quit && !restart) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
        switch(event.type) {
          case SDL_QUIT:
            quit = true;
            break;
          case SDL_KEYDOWN:
            switch(event.key.keysym.sym) {
              case SDLK_ESCAPE:
                quit = true;
                break;
              case SDLK_r:
                restart = true;
                break;
            }
            break;
        }
      }

      uint32_t time = SDL_GetTicks() - startTime;
      *(uint32_t*)(memory + 64) = time;

      int numKeys;
      const Uint8* keyState = SDL_GetKeyboardState(&numKeys);
      uint8_t buttons = 0;
      for(int i = 0; i < 8; ++i) {
        if(keyState[uw8buttonScanCodes[i]]) {
          buttons |= 1 << i;
        }
      }
      memory[0x44] = buttons;
    
      if(hasUpdFunc) {
        verifyM3(runtime.runtime, m3_CallV(updFunc));
      }
      memcpy(audioState.registers, memory + 0x50, 32);

      uint32_t* palette = (uint32_t*)(memory + 0x13000);
      uint8_t* pixels = memory + 120;
      for(uint32_t i = 0; i < 320*240; ++i) {
        pixels32[i] = palette[pixels[i]];
      }
      SDL_UpdateTexture(texture, NULL, pixels32, 320*4);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      uint32_t frame = (uint32_t)((uint64_t)time * 60 / 1000);
      uint32_t offset = time - (uint32_t)((uint64_t)frame * 1000 / 60);
      uint32_t nextFrameTime = (uint32_t)((uint64_t)(frame + 1) * 1000 / 60 + (offset < 4 ? offset : 4));
      uint32_t delay = startTime + nextFrameTime - SDL_GetTicks();
      if(delay < 33) {
        SDL_Delay(delay);
      }
    }

    SDL_CloseAudioDevice(audioDevice);
    m3_FreeRuntime(audioState.runtime.runtime);
    m3_FreeRuntime(runtime.runtime);
  }

  return 0;
}
