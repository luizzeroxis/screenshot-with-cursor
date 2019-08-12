screenshot-with-cursor: screenshot-with-cursor.cpp
	g++ screenshot-with-cursor.cpp -o screenshot-with-cursor.exe -lgdi32 -lgdiplus -static-libgcc -static-libstdc++ -g0 -O3 -fno-exceptions -s