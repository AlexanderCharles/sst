
sst
---
sst ("Simple Screenshot Tool") is a screenshot/snipping program which uses Xorg and xclip.

After installing, bind it to a key. The selection is copied to the clipboard.

It has not been thoroughly tested across different distros and DEs, nor with multiple monitors.
This project will likely not be updated. Feel free to copy the codebase and expand on it.

Features:
- Copies the snippet
- [Suckless](https://suckless.org/philosophy/)-styled
- Small codebase (~350 SLOC), written in C99

Building
--------
Requires xclip, and dev libs for xlib and UUID.

`make`, followed by `make install` to install.
