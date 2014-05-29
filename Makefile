all: remoteenv.so

remoteenv.so:
	gcc -std=c99 -shared -fPIC *.c -o remoteenv.so -ldl

test: all
	curl -XPUT -d "FROM CONSUL" localhost:8500/v1/kv/env.TEST_CONSUL
	LD_PRELOAD=`pwd`/remoteenv.so ruby get.rb
