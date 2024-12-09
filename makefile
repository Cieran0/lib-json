json: src/*.c include/*.h
	gcc src/*.c -Iinclude -Wall -o json -Lexternal/bin -ltokenise -Iexternal/lib-tokenise/include


get_libs: external/lib-tokenise/src/*.c
	$(MAKE) -C external/lib-tokenise
	cp external/lib-tokenise/*.a external/bin/