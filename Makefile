app: main.cpp
	g++ -g -std=c++23 main.cpp -lvulkan -lglfw -o app

run: app
	./app

clean:
	rm -rf app compile_commands.json
