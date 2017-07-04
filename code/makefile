ignored_warnings = -Wno-c++11-compat-deprecated-writable-strings -Wno-format-security
RANDOM := $(shell bash -c 'echo $$RANDOM')

build: FORCE
	mkdir -p ../build
	rm -f ../build/*.so
	sed -n "/struct platform_api/,/};/p" connection_protocol.h | \
		sed '\://: d' | \
		sed '/^[[:space:]]*$$/d' | \
		sed 's/.*\*\(.*\);[[:space:]]*/\1/' | \
		sed '/struct platform_api/ d' | \
		sed '/{/ d' | \
		sed '/};/ d' > ../build/platform_api_names_in_order.txt
	gcc -g $(ignored_warnings) -c github_webhook.cpp -o ../build/github_webhook.o
	gcc -g $(ignored_warnings) -c http_web_server.cpp -o ../build/http_web_server.o
	gcc -g $(ignored_warnings) -c unix_api.cpp -o ../build/unix_api.o
	gcc -g -shared -o ../build/lib_github_webhook$(RANDOM).so \
		../build/github_webhook.o -lm
	gcc -g -shared -o ../build/http_web_server$(RANDOM).so \
		../build/http_web_server.o -lm
	#IMPORTANT(bjorn): The lib_unix_api.so is guaranteed to exist and the server
	# expects it to be the last dll that compiles.
	gcc -g -shared -o ../build/lib_unix_api$(RANDOM).so ../build/unix_api.o -lm
	gcc -g $(ignored_warnings) ../code/unix_platform.cpp -o ../build/server

run: FORCE
	../build/server

debug: FORCE
	open -a XCode debug_only.xcodeproj
FORCE:
