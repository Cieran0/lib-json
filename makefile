all:
	mkdir -p build
	cd build && cmake .. && make
	cp build/libjson.a libjson.a

clean:
	rm -rf build
