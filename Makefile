iproxy: *.c 
	gcc -Wall -o $@ $^ -lev -g

clean:
	rm -f iproxy
.PHONY:clean
