CFLAGS = -I . -I ./ev/ -DHAVE_CONFIG_H
LDFLAGS =

target = uot

$(target) : *.c ev/ev.c
	gcc -o $@ $^ -lm

clean:
	rm -f $(target) 
.PHONY:clean
