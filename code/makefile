ignored_warnings = -Wno-c++11-compat-deprecated-writable-strings -Wno-format-security

build: FORCE
	mkdir -p ../build
	grep -oE "\(platform_.*\)" connection_protocol.h | sed "s/(platform_//g" | sed "s/)//g" > ../build/platform_api_names_in_order.txt
	gcc $(ignored_warnings) -c github_webhook.cpp -o ../build/github_webhook.o
	gcc $(ignored_warnings) -c unix_api.cpp -o ../build/unix_api.o
	gcc -shared -o ../build/lib_github_webhook.so  ../build/github_webhook.o  -lm
	gcc -shared -o ../build/lib_unix_api.so  ../build/unix_api.o  -lm
	gcc $(ignored_warnings) -g ../code/unix_platform.cpp -o ../build/server

run: FORCE
	../build/server

debug: FORCE
	open -a XCode debug_only.xcodeproj
FORCE:
