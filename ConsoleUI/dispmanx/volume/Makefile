CXFLAGS+=-std=c++11 -O3 -fPIC -Wall -DOMX_SKIP64BIT -DRPI3=1 -DVWALL=1 -Wdeprecated-declarations
LDFLAGS+=-L/opt/vc/lib -lbcm_host -lvchostif -lopenmaxil -lbrcmGLESv2 -lbrcmEGL
LDFLAGS+=-L/usr/lib/arm-linux-gnueabihf
LIBS+=-ldl -lpthread -lwiringPi -lpthread
INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/usr/include/arm-linux-gnueabihf

PHONY: clean volume

volume: volume.cpp dispmanx.cpp
	g++ -o volume $(CXFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS) volume.cpp dispmanx.cpp

clean:
	rm -f volume
