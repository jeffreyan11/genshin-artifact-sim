# Genshin Artifact Sim

This is a work in progress.

See the [wiki](https://github.com/jeffreyan11/genshin-artifact-sim/wiki) for more details.

## Usage
`genshin-artifact-sim` uses an interactive command-line interface. After starting the program, type `help` for a full list of commands and their usage.  The sim uses relative paths to find configs, so keep the sim binary and `config/` directory in the same folder.

To compile your own copy: with `g++` installed, clone the repository, navigate to `src/`, and run `make`. The output binary name is `sim` (or `sim.exe` on Windows).

There are several ways to get a C++ compiler on Windows. I use [MSYS2](https://www.msys2.org/).

## Credits

- Collaborators from the KeqingMains Discord, especially srl#2712 and Venatic#3993 for giving me the idea to work on this project.
- [Dimbreath for datamined probabilities](https://github.com/Dimbreath/GenshinData)
- [KeqingMains Theorycrafting Library](https://library.keqingmains.com/)

## Future Work

- Allow input for a current set of artifacts, to see the ROI on further farming.
- Handle characters that scale off of different stats, such as DEF or HP.
- Farm for more than one character at a time.
