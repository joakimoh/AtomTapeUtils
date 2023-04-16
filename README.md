# AtomTapeUtils
This is a set of utilities mainly targeting decoding and encoding of Acorn Atom BASIC programs. Some utilites (like filtering of tape files) could be useful also for other applications. All utilites are run from command line (DOS or Linux). There are many utilities and the easiest way to explain them is to decsribe a few typical usages. 

## You have a tape with old Acorn Atom programs that you have digitalised into a WAV file and you want to extract the programs from it.

The WAV file must be a 44.1 kHz/16-bit/mono PCM file. 
Filter the WAV file *my_tape.wav* to clean it up a bit before trying to decode it.
(This filtering can be skipped if you already have an audio file of excellent quality.)

> filtertape my_tape.wav -o my_filtered_tape.wave

Then extract programs from it

> ScanTape my_filtered_tape.wav -g my_files_dir

For each detected Acorn Atom program file, the following files will be generated and stored in the directory *my_files_dir*:
- <program name>.abc - text file with the BASIC program (looks as it would appear when listed on the Acorn Atom)
- <program name>.dat - hex dump of the same program file (useful if the program includes binary data)
- <program name>.uef - UEF file that can be loaded into an Acorn Atom emulator like Atomulator
- <program name> - MMC file that can be stored on an SD memory card (or in the MMC directory of Atomulator) for use with an AtoMMC device connected to an Acorn Atom
- <program name>.tap - ATM file ("Wouter Ras" format) that can be loaded into an Acorn Atom emulator like the one from Wouter Ras

The program name will be used as the file name. Any detected non-alphanumeric characters will however be replaced with \_XX in the generated file's name where XX is the hex code for the character. If the decoded file is corrupted, then only the .abc and .dat files are generated and the file name base will be *program name*\_incomplete*\_*n1*\_*n2* where n1 and n2 tell which blocks of the program were detected (from block n1 to block n2). If a block is just partially detected, the missing data bytes will be replaced with zeroes in the generated files. Thus, it is possible to recover partially read files to some extent.

## You have written a program on your desktop (myprog.abc) and want to encode it as something that can be run on an emulator or loaded into an Acorn Atom

Convert to EUF format (for loading into emulator)

> abc2uef myprog.abc -o myprog.uef

Convert to MMC format (for loading into emulator or onto memory card of an AtoMMC device)

> abc2mmc myprog.abc -o myprog

Convert to TAP format (for loading into emulator)

> abc2tap myprog.abc -o myprog.tap

Convert to WAV format (for loading into an Acorn Atom via audio cable connected between desktop and Acorn Atom)

> abc2wav myprog.abc -o myprog.wav

Optionally, you could do all this (except for the WAV file generation) with just one command:

> abc2all myprog.abc -g gen_dir

## Other utilities

- abc2dat: Convert an Acorn Atom program text file into a hex dump file (showing how the program will be stored on an Acorn Atom)
- dat2abc: Convert hex dump file into text file with Acorn Atom program
- dat2bin: Convert hex dump file into binary file (useful if you want to disassemble machine code)
- dat2mmc: Convert hex dump file into an MMC file
- dat2uef: Convert hex dump file into  an UEF file
- dat2wav: Convert hex dump file into a WAV file (16-bit PCM WAV for Acorn Atom)
- mmc2abc: Convert MMC file into an Acorn Atom program text file
- mmc2dat: Convert MMC file into a hex dump file
- uef2abc: Convert UEF file into an Acorn Atom program text file
- inspectfile: hex dump of a file content

## Utility program flags
There are many possibilities to tailor especially the tape filtering and tape scannning. Write *utility name* and press enter to get information about the command line flags you can provide to do this tailoring. One useful feature (enabled by flag '-m') is e.g. the ability to generate a WAV file that includes both the original audio and the filtered audio for manual inpspection when you are experiencing difficulties with some tapes (i.e. they are not successfully decoded with ScanTape later on). This WAV file cannot be used by ScanTape though as ScanTape expected only one channel with audio data. You could also turn on logging of detected faults during decoding of a tape (flag '-t') that will tell you at what points in time the decoding fails (like preamble byte #2 read failure).
