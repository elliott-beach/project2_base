from pprint import pprint
from itertools import cycle
import matplotlib.pyplot as plot
import numpy

algorithms = ['rand', 'fifo', 'custom']
program_str = 'program: '

raw_data = open("experiment_data").read().split('\n')

frame_counts = list(map(int, raw_data[1].split()))
frames = cycle(frame_counts)

results = {}

for line in raw_data[2:]:
    line = line.strip()
    if not line:
        continue
    if program_str in line:
        program = line[len(program_str):]
        results[program] = {}
    elif line in algorithms:
        algorithm = line
        results[program][algorithm] = [[], []]
    else:
        results[program][algorithm][0].append(next(frames))
        results[program][algorithm][1].append(list(map(int, line.split())))

for program in ['sort', 'scan', 'focus']:
    current = results[program]
    legends = []
    colors = cycle(['b', 'g', 'r'])
    for alg in algorithms:
        xs, ys = current[alg]
        reads = [y[0] for y in ys]
        writes = [y[1] for y in ys]
        faults = [y[2] for y in ys]
        color = next(colors)
        plot.plot(xs, reads, '-.',c=color)
        plot.plot(xs, writes, '--', c=color)
        plot.plot(xs, faults, c=color)
        legends.append(alg + ' disk reads')
        legends.append(alg + ' disk writes')
        legends.append(alg + ' page faults')
    plot.ylabel('reads, writes, faults')
    plot.xlabel('frames')
    plot.title('Reads, Writes, and Faults for {}'.format(program))
    plot.legend(legends)
    plot.xticks(frame_counts)
    plot.show()
