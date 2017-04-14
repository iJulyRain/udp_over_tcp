CFLAGS = -I . -I ./ev/ -DHAVE_CONFIG_H -g
LDFLAGS =

target = uot

$(target) : *.c ev/ev.c
	gcc -o $@ $^ -lm $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(target) 
.PHONY:clean
