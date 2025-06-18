## 📝 SIC/XE-Assembler-System-Programming

A SIC/XE assembler implemented in C, based on the textbook example in Figure 2.6.  

It generates an object program following the record format specified in Figure 2.8.

### 🔗 1131 NTNU CSIE System Programming Final Project

- Course code: 3N1383701

- Instructor: Prof. Gwan-Hwan Hwang(黃冠寰 教授)

- Textbook used: System Software An Introduction To Systems Programming, by Leland L. Beck 

- My final project notes: [系統程式 期末專題 SIC/XE 組譯器 筆記](https://hackmd.io/@Dylan-Dai/rJlpnliIye)

### 📁 Contents
- `main.c` - program entry point; Pass 1, Pass 2, and object file generation.
- `input.txt` - the SIC/XE assembly source file to be assembled.
- `registers.txt` - register table: maps register names to register numbers.
- `mnemonic.txt` - opcode table: maps instruction mnemonics to format and opcode.
- `object_program.txt` - output file storing the generated object program in textbook format (H/T/M/E records).

## 📦 Object Program Output
Object program corresponds directly to textbook Figure 2.8.

| Input (input.txt) | Output |
|-------------|-----------------|
| ![Input](image/1.PNG) | ![Output](image/2.PNG) |

## 🚀 Getting Started
To compile the program:
```
gcc main.c
```
To assemble the code in `input.txt`:
```
a.exe
```
The program will process `input.txt`, build symbol and opcode tables using `registers.txt` and `mnemonic.txt`, then output the object program into `object_program.txt`.
