build: FORCE
	mkdir -p ../build
	gcc -g ../code/unix_platform.cpp -o ../build/server

run: FORCE
	../build/server

debug: FORCE
	open -a XCode debug_only.xcodeproj
FORCE:
