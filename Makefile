all: inject sleep spin

inject: inject.c code.s /tmp/injectme
	gcc -DUSER=\"$$(id -nu)\" code.s inject.c -o inject
	@[ -e /sys/fs/cgroup/freezer/$$(id -nu) ] || \
		echo "------------now run this: mkdir /sys/fs/cgroup/freezer/$$(id -nu) ----------"

/tmp/injectme: injectme
	cp injectme /tmp/injectme
	
injectme: jmp.o
	objcopy -O binary --only-section=.text jmp.o injectme

jmp.o: jmp.s
	gcc -c jmp.s

clean:
	rm -f /tmp/injectme injectme inject *.o sleep spin

spin: spin.o
	ld spin.o -o spin

spin.o: spin.s
	gcc -c spin.s

.PHONY: clean all
