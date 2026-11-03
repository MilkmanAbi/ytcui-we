# Native C++ client. Use build-native.ps1 for the packaged Windows build.
PREFIX ?= /usr/local
JOBS ?= 2
.PHONY: all test install clean
all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build --parallel $(JOBS)
	cp build/ytcui-we ./ytcui-we
test: all
	ctest --test-dir build --output-on-failure
install: all
	cmake --install build --prefix "$(PREFIX)"
clean:
	cmake --build build --target clean
	rm -f ./ytcui-we
