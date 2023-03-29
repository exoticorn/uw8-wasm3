let U8 = (...a) => new Uint8Array(...a);

let memory = new WebAssembly.Memory({ initial: 4 });
let memU8 = U8(memory.buffer);

let importObject = {
  env: {
    memory
  }
};

let loaderWasm = await Deno.readFile("loader.wasm");
let loader = (await WebAssembly.instantiate(loaderWasm, importObject)).instance;

let platformUw8 = await Deno.readFile("platform.uw8");
console.log("platform.uw8 size: " + platformUw8.byteLength);
memU8.set(U8(platformUw8));
let platformSize = loader.exports.load_uw8(platformUw8.byteLength);
let platformWasm = new ArrayBuffer(platformSize);
U8(platformWasm).set(memU8.slice(0, platformSize));

console.log("Unpacked platform size: " + platformSize);

console.log("First byte: " + U8(platformWasm)[0]);