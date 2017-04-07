iproxy: *.c 
	gcc -o $@ $^

clean:
	rm -f iproxy
.PHONY:clean
