WASM3_C := $(wildcard wasm3/source/*.c)
WASM3_O := $(WASM3_C:.c=.o)

uw8-wasm3: main.o $(WASM3_O) platform.o loader.o wasm-rt-impl.o
	gcc -g -lm -lSDL2 -o uw8-wasm3 $^

run: uw8-wasm3 .PHONY
	./uw8-wasm3 skipahead.uw8

run-ts:
	deno run --allow-read main.ts

wasm3/source/%.o: wasm3/source/%.c
	gcc -g -O2 -c -o $@ $<

%.o: %.c platform.h wasm-rt.h wasm-rt-impl.h
	gcc -g -O2 -c -o $@ $<

clean:
	rm uw8-wasm3 main.o $(WASM3_O)

.PHONY: