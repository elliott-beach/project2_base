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
	@echo 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
	@for program in sort scan focus ; do \
	    echo program: $$program ; \
	    for algorithm in rand fifo custom; do \
		echo $$algorithm ; \
	        for frame_count in 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 ; do \
	            ./virtmem 100 $$frame_count $$algorithm $$program | grep -v 'result\|length\|sum'; \
	        done \
	    done \
	done

experiment_data: virtmem
	make experiment --no-print-directory > experiment_data

figures: experiment_data graphs.py
	mkdir -p figures
	python3 graphs.py

report: # requires https://github.com/prasmussen/gdrive installed
	gdrive export -f 1gmnUy7Zuh2qBM5WYFcedKl3oWvlFLMmZOiL31bTfcMc
	mv *Report.pdf report.pdf

publish: report
	tar -cvf project.tar report.pdf *.c *.h Makefile graphs.py

clean:
	rm -f *.o virtmem experiment_data figures/*
