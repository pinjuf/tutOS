Chapter 21 - ELF Loader 

When we started with usermode, we just compiled into
a pure binary format, which would be loaded entirely
and had to have its entry point at its start. When
using the Executable & Linkable Format, this is no
longer necessary, as now, programs are loaded in sections.
For now, we will only support static, non-PIE programs.

FAQ:
    1) Why don't you do anything with the section headers/symbol tables?
