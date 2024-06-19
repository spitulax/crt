<h1 align="center">Commands Run Today</h1>

**C**ommands **R**un **T**oday
is a 🚀 **blazing fast** 🚀 command-line utility that counts the number of times you have run a command today
using shell history.

## Shell Supported
- [Atuin](https://github.com/atuinsh/atuin)
- [Fish](https://fishshell.com)

## Usage

Crt must be executed at the first time user logs into the interactive shell on the current day to sync the crt database with your shell history.
The easiest method to ensure this is to execute crt on your shell startup script.
If you are not planning to use this often you can add the flag `--update` which will sync the crt database but it's slower.

Place this in your shell startup script:

```console
crt <your shell>
```

## Building

### Nix (Flake)

```console
$ nix build
$ ./result/bin/crt --help
```

### Other

```console
$ ./deps/build.sh && meson setup build && meson compile -C build
$ ./build/crt --help
```

## Caveat

Fish does not update the history if you run the same command as the previous command,
and if you run a sequence of commands multiple times, fish will truncate down the history on the fly which decrease the count
and may get crt without `--update` to count the history as negative, which will be displayed as 0.

## Note

The way I build the sqlite3 library is kind of hacky, and it gets even hackier in the nix package declaration.
I tried to get meson to build it I just can't get it to work. I literally just want to run a couple of shell commands.
I've tried some solutions, most of them don't work and some of them that do are not less janky than this.
~Every single build system is like turing complete, yet some should-be-trivial tasks are just made unnecessarily convoluted.~
