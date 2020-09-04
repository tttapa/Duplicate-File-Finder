# Duplicate File Finder

Command line utility that recursively scans a directory for duplicate files 
(i.e. files with the same content).
This can be really useful to cleanup backups or to remove unnecessary copies 
of files.

## Usage

```sh
$ dupfind --help

Find duplicate files.

Usage: dupfind [options] [directory]

If no directory is provided, the current working directory is used.

Allowed options:
  -h [ --help ]            Print this help message.
  -v [ --version ]         Print the version number.
  -d [ --dir ] <directory> The directory to process.
  -i [ --include ] <regex> Include only files that match these patterns.
  -e [ --exclude ] <regex> Exclude any files that match these patterns.
  -s [ --sort-size ]       Sort the output by file size.
  --include-empty-files    Include all empty files.
```

### Examples

Find all duplicate files in the current folder:

```sh
dupfind
```

Find all duplicate WAV and MP3 files in `~/Music` and `~/Audio`, ignoring files 
named `cello.wav`:

```sh
dupfind ~/Music ~/Audio --include '.*\.wav' --include '.*\.mp3' --exclude '.*/cello.wav'
```

Find all duplicate files in the home folder, sorting the output by file size:

```sh
dupfind ~ --sort-size
```

## Installation

If you're on Debian or Ubuntu, you can download the `.DEB` package from the
[releases](https://github.com/tttapa/Duplicate-Finder/releases) page. It can
be installed through the Software Center or through the command line:

```sh
sudo apt install ~/Downloads/Duplicate-File-Finder-0.0.1-Linux.deb
```

You can also download the `.TAR.GZ` archive. Simply unzip it, and move the 
executable to a folder that's in your path:

```sh
tar xzf Duplicate-File-Finder-0.0.1-Linux.tar.gz
sudo mv bin/dupfind /usr/local/bin
```

You might have to install some dependencies, like `libssl1.1` and 
`libboost-program-options1.71.0`.

## Build from source

```sh
# Install dependencies
sudo apt install -y libssl-dev libboost-program-options-dev
# Download this repository
git clone https://github.com/tttapa/Duplicate-Finder
cd Duplicate-Finder
# Configure
cmake . -Bbuild -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release
# Compile
cmake --build build
# Install
sudo cmake --build build --target install
```

## How it works

The program builds two dictionaries: one that keeps track of the file size and
the file path of each file, and a second one that keeps track of the SHA-1
hash of some of the files. If the sizes of two files are different, they
cannot be duplicates.
If the sizes of two files are the same, however, their hashes are computed and
compared. Finally, all files with hashes that occur multiple times in the
second dictionary are sorted and printed.
