all: bin

bin: src
	find src -type f -perm /a+x -exec cp {} bin \;

src: src/ lib
	$(MAKE) -C src

lib: lib/
	$(MAKE) -C lib
