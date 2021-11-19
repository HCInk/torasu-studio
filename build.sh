#!/bin/bash
set -e

function build_runMake() {
	if [ -f  "$(command -v nproc)" ]; then
		threads=$(nproc --all)
		echo "Building multithreaded ($threads)..."
		make --jobs=$threads
	else
		echo "Building singlethreaded..."
		make
	fi
}

if [ -n "$1" ]; then

	if [[ $1 == *"debug"* ]]; then
		echo "Building debug..."
		cd build
		cmake -Wno-dev -DCMAKE_BUILD_TYPE=Debug ../
		build_runMake
		cd ..
	fi

	if [[ "$1" == *"release"* ]]; then
		echo "Building release..."
		cd build
		cmake -Wno-dev -DCMAKE_BUILD_TYPE=Release ../
		build_runMake
		cd ..
	fi

	if [[ "$1" == *"install"* ]]; then
	
		echo "Installing TORASU-STUDIO..."
			
		mkdir -p build
		cd build

		build_runMake
		if [ $(uname) == "Darwin" ] || [ "$2" == "nosudo" ]; then
			echo "Installing TORASU-STDUIO as user..."
      		make install
    	else
			echo "Installing TORASU-STDUIO as super-user..."
			sudo make install
    	fi

		cd ..
	fi
	
	if [[ "$1" == *"wasm"* ]]; then
	
		echo "Installing TORASU [WASM]..."
			
		mkdir -p build/cross/wasm
		cd build/cross/wasm
		emcmake cmake -Wno-dev -DCMAKE_BUILD_TYPE=Release ../../../
		build_runMake
		cd ../../..
	fi

	if [[ "$1" == *"wasmrun"* ]]; then
		node ./wasm-test-server build/cross/wasm
	fi

	if [[ "$1" == *"delbuild"* ]]; then
	
		echo "Deleting build-folder..."
		rm -r build
	fi
	
	if [[ "$1" == *"help"* ]]; then 

		echo "Recieved arguments: \"$1\""
		echo "Available arguments: "
		echo "	nosudo      - Dont use sudo for installs"
		echo "	debug 		- Apply debug config"
		echo "	release 	- Apply release config"
		echo "	install 	- Installs current configuration"
		echo "	wasmbuild 	- Builds wasm-version into build/cross/wasm/"
		echo "	delbuild 	- Deletes all buld files (build/)"
		echo "No arguments will just run a normal build."
		echo "You can combine args as you wish, they are applied in the order listed above."
		echo "(order they are written in the argument dont matter, as long as they are in the first argument)."
		echo "For example \"install-release\" will build a release and install it afterwards."

	fi

else

	mkdir -p build
	cd build
	cmake -Wno-dev ../
	make -j
	cd ..

fi