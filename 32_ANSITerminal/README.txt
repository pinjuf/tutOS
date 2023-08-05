Chapter 32 - ANSI Terminal

For now, our terminal has been rather "dumb", only supporting
basic special control codes (so-called C0-codes). However, you
have probably seen programs that display seemingly impossible
graphics, with jumping cursors, diverse colours, etc. If you
haven't, try out some (n)curses/other programs/games, such as
NetHack. This special behaviour is achieved with ANSI escape
codes, which start with a "\e". In our case, we will be
implementing Control Sequence Introducers (CSIs), which all
start with "\e[".

FAQ:
