#!/bin/bash

project_name="project_name"

# Check if project_name has been set properly
if [ "$project_name" = "project_name" ]; then
    echo "ERROR: Please set the 'project_name' variable in build.sh to the name of your project before running this script."
    exit 1
fi

# Function to create directories
create_directory() {
	if [ ! -d "$1" ]; then
		mkdir -p "$1"
		echo "[INFO] created directory \`$1\`"
	fi
}

# Function to compile the project
compile_gui() {
	local project_src="./src"
	local project_build="./build"
	local project_include="./include"

	# Compile gui
	clang -Wall -Wextra -g -DHOTRELOAD \
		-I$project_include/ \
		-I$project_build/ \
		-fPIC -shared -o $project_build/lib${project_name}.dylib.new $project_include/raylib/macos/libraylib.dylib $project_src/gui.c \
                $project_include/raylib/macos/libraylib.dylib \
		-lm -ldl -lpthread

	# Replace the old shared library with the new one
	mv $project_build/lib${project_name}.dylib.new $project_build/lib${project_name}.dylib

	echo "[INFO] GUI compiled successfully"
}

compile_main() {
	local project_src="./src"
	local project_build="./build"
	local project_include="./include"

        if [ ! -d "$project_build" ]; then
            mkdir -p "$project_build"
            echo "[INFO] Created build directory: $project_build"
        fi

	# Compile main
	clang -Wall -Wextra -g -DHOTRELOAD \
		-I$project_include/ \
		-I$project_build/ -o $project_build/${project_name} \
		$project_src/main.c $project_src/hotreload.c $project_include/raylib/macos/libraylib.dylib \
		-framework Security -framework CoreServices \
		-rpath ./ -rpath $project_build -rpath $project_include/raylib/macos \
		-lm -ldl -lpthread

	echo "[INFO] Project compiled successfully"
}

watch_gui_changes() {
	local last_gui_checksum=""
	while true; do
		local gui_files=$(find ./src -name 'gui.*')
		local gui_checksum=$(md5 -q $gui_files)

		if [ "$gui_checksum" != "$last_gui_checksum" ]; then
			last_gui_checksum="$gui_checksum"
			compile_gui
		fi
		sleep 1 # Adjust sleep duration as needed
	done
}

compile_together() {
	local project_src="./src"
	local project_build="./build"
	local project_include="./include"

        if [ ! -d "$project_build" ]; then
            mkdir -p "$project_build"
            echo "[INFO] Created build directory: $project_build"
        fi

	clang -Wall -Wextra -g \
		-I$project_build/ \
		-I$project_include/ \
		-o $project_build/${project_name} \
		$project_include/libraylib.a \
		$project_src/gui.c $project_src/main.c \
		-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL \
		-framework Security -framework CoreServices \
         	-lm -ldl -lpthread

	echo "[INFO] Project compiled successfully"
}

compile_web() {
  emcc -o index.html src/main_web.c src/gui.c \
       -Os -Wall -I. -I./include/ -L. \
       -s USE_GLFW=3 -s ASYNCIFY --shell-file ./shell.html -DPLATFORM_WEB

  mv index.* www/
}

echo "#ifndef BUILD_H" > "./include/build.h"
echo "#define BUILD_H" >> "./include/build.h"
echo "// Project Name"
echo "#define PROJECT_NAME \"${project_name}\"" >> "./include/build.h"
echo "#endif // BUILD_H" >> "./include/build.h"
echo "[INFO] Generated build header file"

case "$1" in
    hot) watch_gui_changes ;;
    release) compile_together ;;
    web) compile_web ;;
    *) compile_gui ; compile_main ;;
esac
