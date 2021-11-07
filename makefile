all: game

game: main.c makefile
	clang -g -w -lglfw -framework OpenGL main.c -o game

run: game
	./game
	
debug: game
	lldb game


