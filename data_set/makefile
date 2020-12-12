COMMON=-O2 -I./include -L. -std=c++11 -stdlib=libc++ -mavx -pthread

all:
	clang++ $(COMMON) generate_data.cpp     -lmujoco200 -lglfw.3 -o generate_data 
