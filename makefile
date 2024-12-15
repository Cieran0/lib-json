all:
	mkdir -p build
	cd build && cmake .. && make
	cp build/json json

clean:
	rm -rf build
