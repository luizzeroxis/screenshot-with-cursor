release: screenshot-with-cursor.cpp
	g++ screenshot-with-cursor.cpp -o screenshot-with-cursor.exe -Wall -Werror -lgdi32 -lgdiplus -static-libgcc -static-libstdc++ -g0 -O3 -s

dev: screenshot-with-cursor.cpp
	g++ screenshot-with-cursor.cpp -o screenshot-with-cursor.exe -Wall -lgdi32 -lgdiplus -static-libgcc -static-libstdc++ -g