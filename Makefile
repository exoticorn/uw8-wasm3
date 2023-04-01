WASM3_C := $(wildcard wasm3/source/*.c)
WASM3_O := $(WASM3_C:.c=.o)

uw8-wasm3: main.o $(WASM3_O)
	gcc -g -lm -lSDL2 -o uw8-wasm3 $^

run: uw8-wasm3 .PHONY
	./uw8-wasm3 never_sleeps.uw8

run-ts:
	deno run --allow-read main.ts

wasm3/source/%.o: wasm3/source/%.c
	gcc -g -O2 -c -o $@ $^

main.o: main.c
	gcc -g -O2 -c -o main.o main.c

clean:
	rm uw8-wasm3 main.o $(WASM3_O)

.PHONY: