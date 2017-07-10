ignored_warnings = -Wno-c++11-compat-deprecated-writable-strings -Wno-format-security -Wno-write-strings
RANDOM := $(shell bash -c 'echo $$RANDOM')

build: FORCE
	mkdir -p ../build
	rm -f ../build/*.so
	rm -f ../build/*.so.temp
	sed -n "/struct platform_api/,/};/p" connection_protocol.h | \
		sed '\://: d' | \
		sed '/^[[:space:]]*$$/d' | \
		sed 's/.*\*\(.*\);[[:space:]]*/\1/' | \
		sed '/struct platform_api/ d' | \
		sed '/{/ d' | \
		sed '/};/ d' > ../build/platform_api_names_in_order.txt
	gcc -g $(ignored_warnings) -c http_web_server.cpp -o ../build/http_web_server.o
	gcc -g $(ignored_warnings) -c unix_api.cpp -o ../build/unix_api.o
	gcc -g -shared -o ../build/http_web_server$(RANDOM).temp \
		../build/http_web_server.o -lm 
	#IMPORTANT(bjorn): The lib_unix_api.so is guaranteed to exist and the server
	# expects it to be the last dll that compiles.
	gcc -g -shared -o ../build/lib_unix_api$(RANDOM).temp ../build/unix_api.o -lm
	#TODO(bjorn): This works on linux and osx.
	# Implying that what I really need to do is to make sure that the file
	# is completely written before I touch it. I should probably make sure that
	# the server can handle this on its own.
	for j in ../build/*.temp; do mv -- "$$j" "$${j%.temp}.so"; done
	gcc -g $(ignored_warnings) ../code/unix_platform.cpp -o ../build/server -ldl

run: FORCE
	../build/server

debug: FORCE
	open -a XCode debug_only.xcodeproj
FORCE:
