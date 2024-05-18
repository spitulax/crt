<h1 align="center">Commands Run Today</h1>

**C**ommands **R**un **T**oday
is a 🚀 **blazing fast** 🚀 command-line utility that counts the number of times you have run a command today
using shell history.

## Shell Supported
- [Fish](https://fishshell.com)

## Usage

```bash
crt --help
```

## Building

### Nix (Flake)

```bash
nix build
./result/bin/crt --help
```

### Other

```bash
make RELEASE=1
make RELEASE=1 install # Put the binary to ~/.local/bin
```
