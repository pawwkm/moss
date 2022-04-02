Moss
====

![Screenshot](screenshot.png)

This is a proof of concept of a Modal text editor for my personal use. Supply `moss.exe` with paths to the files you want to open. This is temporary.

# Modes
```peg
NORMAL <- TAB
        / INSERT
        / search
        / operator modifier object
        / repetition? (shorthand_operator / double_operator / operator? motion)

repetition <- [0-9]+

search <- ('/' / '?') (!enter char)* enter
```

## NORMAL
Editing in NORMAL mode works on spans of texts specified by `motion`s and `object`s now referred to as text selection. If no text selection be found the operator does nothing.

### Operators
```peg
operator <- 'd'  / 'c'  / 'y'
```

* `d` deletes the text specified by the motion or object. 
* `c` same as `d` but enters INSERT mode afterwards.
* `y` yanks the text selection.

### Shorthand operators
```peg
shorthand_operator <- 'C' / 'D'
```

* `C` is equivalent to `c$`.
* `D` is equivalent to `d$`.

### Double operators
```peg
double_operator <- 'dd' / 'cc' / 'yy' / 'gg'
```

* `dd` deletes the whole line.
* `cc` same as `dd` but enters INSERT mode afterwards.
* `yy` yanks the whole line but the cursor stays.
* `gg` is a shorthand for `1G`.

### Motions
```peg
motion <- 'h' / 'j' / 'k' / 'l' / '^' / '$' / 'w' 
        / 'e' / 'b' / 'G' / 'n' / 'N' 
        / [fFtT] .
```

If `motion`s are used with an `operator` it selects the text from where the cursor was before the `motion` and where it ended up. If `motion`s have `repetition` then the `motion` is repeated x times. 

* `h` moves the cursor to the previous character on the current line.
* `j` moves the cursor to the next line and the current column is prefered.
* `k` moves the cursor to the previous line and the current column is prefered.
* `l` moves the cursor to the next character on the current line.
* `^` moves the cursor to the first non-space character on the current line.
* `$` moves the cursor to the end of the current line.
* `w` moves the cursor to the start of the next token. 
* `e` moves the cursor to the end of the next token.
* `b` moves the cursor to the start of the previous token.
* `G` moves the cursor to the last line and the prefered column is ignored.
* `H` moves the cursor to the line just below `LINE_SCROLL_PADDING`.
* `M` moves the cursor to the line in the middle of the view.
* `L` moves the cursor to the line just above `LINE_SCROLL_PADDING`.
* `n` moves the cursor to the first character of the next search result. 
* `N` moves the cursor to the first character of the previous search result. 
* `f` moves the cursor to the first occurrence of the given character to the right of the cursor on the same line.
* `F` moves the cursor to the previous occurrence of the given character to the left of the cursor on the same line.
* `t` moves the cursor to the character before the first occurrence of the given character to the right of the cursor on the same line.
* `T` moves the cursor to the character after the previous occurrence of the given character to the left of the cursor on the same line.

### Objects
```peg
object   <- 'w' / '[' / ']' / '{' / '}' / '(' / ')' / '"' / "'" / '`' / '<' / '>'
modifier <- 'a' / 'i'
```

* `w` selects the token the cursor is on.
* `[` `]`, `{` `}`, `(` `)`,  `"`,  `'`, `\``, `<` `>` selects texts inside a matching pair.

* `i` If inside a `w` then all its characters are selected otherwise all but the pair are selected.
* `a` Selects the object and the space around it.

## INSERT
```peg
INSERT <- ('i' / 'I' / 'a' / 'A' / 'o' / 'O') (!escape .)* escape 
```

* `i` enters INSERT mode at cursor.
* `I` enters INSERT mode at the first non-space character on the current line.
* `a` enters INSERT mode at the next character on the current line.
* `A` enters INSERT mode at the end of the current line.
* `o` insert line below, `j` then `i`.
* `O` insert line above, `k` then `i`.
* `\b` deletes the character to the left of the cursor and goes `h`. If there are no character to the left to delete then the current line and the previous line are merged.
* `\x7F` deletes the character to the right of the cursor. If there are no more characters to the right to delete then the current line and the next are merged.
* `\r` splits the line at the at the cursor.
* `.` is inserted at the cursor.
* `escape` enters NORMAL mode.

## TAB
```peg
TAB <- '\t' [hjHJklKLfgdsC]* escape
```

* `h` moves the active view to the left.
* `j` moves the active view to the right.
* `H` moves the active tab to the left.
* `J` moves the active tab to the right.
* `k` moves the active view in a new tab to the left. 
* `l` moves the active view in a new tab to the right.
* `K` moves the active view in existing tab to the left.
* `L` moves the active view in existing tab to the right.
* `f` activate left hand side view in active tab.
* `g` activate right hand side view in active tab.
* `d` activate right hand side tab.
* `s` activate left hand side tab.
* `C` close active tab.
* `escape` enters NORMAL mode.

# File encodings
Files opened are expected to be ASCII encoded.

# Rendering
The rendering is done entirely using 

* [SDL_RenderFillRect](https://wiki.libsdl.org/SDL_RenderFillRect)
* [SDL_RenderCopy](https://wiki.libsdl.org/SDL_RenderCopy)
* [SDL_RenderDrawLine](https://wiki.libsdl.org/SDL_RenderDrawLine)

## Font rendering
The ASCII portion of [cozette.c](src/renderer/cozette.c) is used for rendering text. In the case of non-ASCII characters a ? where the colors are inverted is used.

# GPU usage
Rendering is GPU accelerated but no frames are drawn unless resizing, typing or
something simmilar happens that requires drawing a new frame.

# Building
## Windows
1. Open up the [Visual Studio Developer Command Prompt](https://docs.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2019) with the `x64` environment.
2. Navigate to the root directory.
3. Run `build.bat`.

`moss.exe` should sit in `root/bin/`.

# Dependencies
* I use the a subset of the [Cozette](https://github.com/slavfox/Cozette) font. Check out [cozette.c](src/renderer/cozette.c) for more details.
* [SDL2](https://www.libsdl.org/).
