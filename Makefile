CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

first_app.out: *.cpp *.hpp
	g++ $(CFLAGS) -o first_app.out *.cpp $(LDFLAGS)

.PHONY: test clean

test: first_app.out
	./first_app.out

clean:
	rm -f first_app.out