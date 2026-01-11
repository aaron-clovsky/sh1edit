# Insert/Extract files to/from Silent Hill for PS1

## CREDIT

Based on information from the [Silent Hill Decompilation Project](https://github.com/Vatuu/silent-hill-decomp).
 
## BUILD INSTRUCTIONS

- On Linux
  - Install a C compiler if one is not present
   (Run ```sudo apt install build-essential``` on Debian distros)
  - ```cd``` to this repo's directory
  - Run ```make```

## DESCRIPTION

Program that simplifies performing modifications to Silent Hill game files. 
 
A list of file offsets and sizes can be found [here](https://github.com/Vatuu/silent-hill-decomp/blob/d79d21d149f7a5a8466eb06be484b8504959cd8c/src/main/filetable.c.USA.inc).

## EXAMPLE

If ```sh1.cue``` and ```sh1.bin``` are the Silent Hill image files.

Extract and decrypt '1ST/BODYPROG.BIN'

``` ./bin/sh1edit ext ./sh1.bin 207 674560 ./BODYPROG.BIN ```

Patch '1ST/BODYPROG.BIN' for 60fps:
```
xxd -p ./BODYPROG.BIN | tr -d '\n' | sed 's/345c438ef08fa48c/345c438e01000424/' | xxd -p -r > ./BODYPROG.TMP
xxd -p ./BODYPROG.TMP | tr -d '\n' | sed 's/7097638ef08f048e/7097638e01000424/' | xxd -p -r > ./BODYPROG.MOD
```

Insert modified '1ST/BODYPROG.BIN'

``` ./bin/sh1edit ins ./sh1.bin 207 ./BODYPROG.MOD ./sh1_mod.bin ```

Create matching .cue

```
cp sh1.cue sh1_mod.cue
sed -i 's/sh1.bin/sh1_mod.bin/g' sh1_mod.cue
```

## NOTES

- Use the ```--recursive``` option when cloning, like this:

```git clone --recursive https://github.com/aaron-clovsky/sh1edit```

- ```make style``` is implemented to keep code within style guidelines,
it requires clang-format-21, run the following to install:
```
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 21
sudo apt install clang-format-21
```

- ```make lint``` is implemented to help check for warnings it
requires: gcc, g++, clang, clang++ and cppcheck

## LICENSE
This software is licensed under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html).
