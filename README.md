# The mini programming language

This is a general-purpose, low-level programming language that is created by educational purpose.

## Compile

At first, close this repository with `--recursive` option and enter the repository.

```
git clone --recursive https://github.com/pogyomo/mini.git
cd mini
```

Then, create a `build` directory, and initialize it with `cmake`.

```
mkdir build && cd build && cmake ..
```

Finally, run `make` to build executable binary.

```
make
```

## Usage

Run below command to obtain executable.

```
mini FILENAME -o OUTPUT
```

See `mini -h` for more option.
