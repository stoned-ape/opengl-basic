all: game

flags=-g -Wall -Wextra -fsanitize=address -fsanitize=undefined 
glflags=-lglfw -framework OpenGL

game: main.c makefile
	clang $(flags) $(glflags) main.c -o game

run: game
	./game -p
	
debug: game
	lldb game


