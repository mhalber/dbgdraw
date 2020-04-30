## Work in progress

# dbgdraw

dbgdraw is intended to be a small immediate mode library for putting simple graphics on the screen. Below
you can see some examples of what can be achieved in dbgdraw.

![Overview](images/overview.png)

### Features

- Written in C99 with minimal, optional dependencies (c stdlib, stb_truetype.h )
- Validation - optionally enable asserts that will inform users on api misuse

### Current issues

- No full UTF-8 support, just latin + greek character ranges
- Only backend is OpenGL 4.5
- MSVC does not compile this code atm.

