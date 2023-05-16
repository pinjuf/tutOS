all: bin

bin: src lib
	# Copy all executables into /bin
	find src -type f -perm /a+x -exec cp {} bin \;

src: src/
	$(MAKE) -C src

lib: lib/
	$(MAKE) -C lib
