CXX=g++
BUILD_TYPE=Test
TEST_FILTER='*'
INTERNAL=false

# Dependencies variable
HOST=x86_64-linux-gnu

test: build
	cd target/src/test/cpp && ./fortissimo_test --gtest_filter=$(TEST_FILTER)

build: build-configure target/
	cd target/ \
		&& make -j4 \
	;

debug-test: build
	cd target/src/test/cpp \
		&& gdb --args ./fortissimo_test --gtest_filter=$(TEST_FILTER) \
	;

target/configure.success:
	cd target/ \
		&& cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
			-DCMAKE_CXX_COMPILER=$(shell which ${CXX}) ../ \
	;
	echo 'success' > target/configure.success

build-configure: dependencies target/ target/configure.success
configure: mopclean dependencies target/ target/configure.success

lib/dependencies.success:
	cd lib/ && make INTERNAL=$(INTERNAL) HOST=$(HOST) all
	echo 'success' > lib/dependencies.success

dependencies: lib/dependencies.success

dependencies-download-only:
	cd lib/ && make INTERNAL=$(INTERNAL) HOST=$(HOST) download-only

format:
	clang-format -i --style=file\
		$(wildcard src/main/cpp/*.cpp src/main/cpp/*.h) \
		$(wildcard src/main/cpp/*/*.cpp src/main/cpp/*/*.h) \
		$(wildcard src/main/cpp/*/*/*.cpp src/main/cpp/*/*/*.h) \
		$(wildcard src/main/cpp/*/*/*/*.cpp src/main/cpp/*/*/*/*.h) \
		$(wildcard src/test/cpp/*.cpp src/test/cpp/*.h) \
		$(wildcard src/test/cpp/*/*.cpp src/test/cpp/*/*.h) \
		$(wildcard src/test/cpp/*/*/*.cpp src/test/cpp/*/*/*.h) \
		$(wildcard src/test/cpp/*/*/*/*.cpp src/test/cpp/*/*/*/*.h) \
		$(wildcard example/src/main/cpp/*.cpp example/src/main/cpp/*.h) \
		$(wildcard example/src/main/cpp/*/*.cpp example/src/main/cpp/*/*.h) \
	;

stealth-format:
	stealth-clang-format -i \
		$(wildcard src/main/cpp/*.cpp src/main/cpp/*.h) \
		$(wildcard src/main/cpp/*/*.cpp src/main/cpp/*/*.h) \
		$(wildcard src/main/cpp/*/*/*.cpp src/main/cpp/*/*/*.h) \
		$(wildcard src/main/cpp/*/*/*/*.cpp src/main/cpp/*/*/*/*.h) \
		$(wildcard src/test/cpp/*.cpp src/test/cpp/*.h) \
		$(wildcard src/test/cpp/*/*.cpp src/test/cpp/*/*.h) \
		$(wildcard src/test/cpp/*/*/*.cpp src/test/cpp/*/*/*.h) \
		$(wildcard src/test/cpp/*/*/*/*.cpp src/test/cpp/*/*/*/*.h) \
		$(wildcard example/src/main/cpp/*.cpp example/src/main/cpp/*.h) \
		$(wildcard example/src/main/cpp/*/*.cpp example/src/main/cpp/*/*.h) \
	;

target/:
	mkdir -p target/

clean:
	[ -f target/Makefile ] && ( cd target/ && make clean ) || true;

mopclean:
	rm -rf target/

mrclean: mopclean
	cd lib && make mrclean
	rm -f lib/dependencies.success
