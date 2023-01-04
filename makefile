all: ping watchdog better_ping
ping: ping.c
	gcc ping.c -o parta
watchdog: watchdog.c
	gcc watchdog.c -o watchdog
better_ping: better_ping.c
	gcc better_ping.c -o partb

new: clean all

clean:
	rm -f *. ping watchdog parta partb
