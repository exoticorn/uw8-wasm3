WASM3_C := $(wildcard wasm3/source/*.c)
WASM3_O := $(WASM3_C:.c=.o)

run: wasm3-test .PHONY
	./wasm3-test

run-ts:
	deno run --allow-read main.ts

wasm3-test: main.o $(WASM3_O)
	gcc -g -lm -lSDL2 -o wasm3-test $^

wasm3/source/%.o: wasm3/source/%.c
	gcc -g -O2 -c -o $@ $^

main.o: main.c
	gcc -g -O2 -c -o main.o main.c

clean:
	rm wasm3-test main.o $(WASM3_O)

.PHONY: