# Chi Programming Language

Chi is a new systems programming language that builds on the foundations of C while adding support for modularity through modules.

## Features

- Performance - Compiles to C for high performance
- Modularity - Module system with signatures, skeletons, implementations and aliases
- Interoperability - Seamless integration with existing C code

## Getting Started

### Prerequisites

- C compiler
- Make

### Installing

```
git clone https://github.com/ghuter/chi
cd chi
make
```

This will build the `chic` executable which can compile .chi files to C.

### Usage

To compile a .chi file:

```
./chic source.chi -o output.c 
```

## Examples

For code examples, see the [examples directory](tests/tests-analyser).

## License

This project is licensed under the [MIT](LICENSE.md).
