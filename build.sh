if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101306172_101265573_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101306172_101265573_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101306172_101265573_EP_RR.cpp