all: injectLib.so logdemon test

SOURSE_DIR=src
BUILD_DIR=build

injectLib.so:
	gcc -shared -fPIC -o ${BUILD_DIR}/injectLib.so ${SOURSE_DIR}/injectLib.c -ldl
logdemon:
	gcc ${SOURSE_DIR}/logdemon.c -o ${BUILD_DIR}/logdemon -lrt
test:
	gcc ${SOURSE_DIR}/test.c -o ${BUILD_DIR}/test
clean:
	rm -f ${BUILD_DIR}/injectLib.so ${BUILD_DIR}/logdemon ${BUILD_DIR}/test