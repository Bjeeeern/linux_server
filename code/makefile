ignored_warnings = -Wno-c++11-compat-deprecated-writable-strings -Wno-format-security

build: FORCE
	mkdir -p ../build
	#TODO(bjorn): new names for new .so files
	sed -n "/struct platform_api/,/};/p" connection_protocol.h | \
		sed 's/.*\*\(.*\);/\1/' | \
		sed '/struct platform_api/ d' | \
		sed '/{/ d' | \
		sed '/};/ d' > ../build/platform_api_names_in_order.txt
	gcc -g $(ignored_warnings) -c github_webhook.cpp -o ../build/github_webhook.o
	gcc -g $(ignored_warnings) -c unix_api.cpp -o ../build/unix_api.o
	gcc -g -shared -o ../build/lib_github_webhook.so ../build/github_webhook.o -lm
	gcc -g -shared -o ../build/lib_unix_api.so ../build/unix_api.o -lm
	gcc -g $(ignored_warnings) ../code/unix_platform.cpp -o ../build/server

run: FORCE
	../build/server

debug: FORCE
	open -a XCode debug_only.xcodeproj
FORCE:
