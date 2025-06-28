all: forth

forth: *.cpp *.hpp
	g++ -O2 -march=native -flto -DNDEBUG -fno-omit-frame-pointer -Wall -Wextra -Werror -std=c++20 -fvisibility=hidden -fPIC *.cpp -o forth