
virtmem: main.o page_table.o disk.o program.o
	gcc main.o page_table.o disk.o program.o -o virtmem

main.o: main.c
	gcc -Wall -g -c main.c -o main.o

page_table.o: page_table.c
	gcc -Wall -g -c page_table.c -o page_table.o

disk.o: disk.c
	gcc -Wall -g -c disk.c -o disk.o

program.o: program.c
	gcc -Wall -g -c program.c -o program.o

experiment: virtmem
	@echo "reads writes faults"
	@for program in sort scan focus ; do \
	    echo program: $$program ; \
	    for algorithm in rand fifo ; do \
		echo $$algorithm ; \
	        for frame_count in 10 20 40 80 100 ; do \
	            ./virtmem 100 $$frame_count $$algorithm $$program | grep -v 'result\|length\|sum'; \
	        done \
	    done \
	done

clean:
	rm -f *.o virtmem
