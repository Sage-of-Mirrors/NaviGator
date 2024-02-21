# bStream - A Binary Reader/Writer for C++
bStream is a header only library for reading and writing big/little endian binary files in C++.

The library contains two classes, `CFileStream` for reading and writing 'physical' files and `CMemoryStream` which is for creating and modifying files in memory. `CMemoryStream` has two constructors, one for generating a new buffer and one for reading from a pre-existing buffer, when made using a pre-existing buffer the stream will not automatically expand as it would when generating a new buffer. 

## Usage
To include bStream in your project simply put `#define BSTREAM_IMPLEMENTATION` in _one_ of the files where you are including bStream. Alternatively the provided bstream.cpp can be added to your project's files and it will handle this for you.
