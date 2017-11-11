from pprint import pprint
import matplotlib.pyplot as plot
import numpy

ALGORITHMS = ['rand', 'fifo', 'custom']

mapp = {}
data = open("experiment_data").read().split('\n')
frame_counts = list(map(int, data[1].split()))

i = 0
current = None
algorithm = None
for line in data[2:]:
    line = line.strip()
    if not line:
        continue
    program_str = 'program: '
    if program_str in line:
        program = line[len(program_str):]
        mapp[program] = {}
        current = mapp[program]
    elif line in ALGORITHMS:
        algorithm = line
        current[algorithm] = [[], []]
    else:
        f = frame_counts[i % len(frame_counts)]
        i += 1
        reads, writes, faults = list(map(int, line.split()))
        current[algorithm][0].append(f)
        current[algorithm][1].append((reads,writes, faults))

colors = ['b', 'g', 'r']
for program in ['sort', 'scan', 'focus']:
    current = mapp[program]
    for i, key in enumerate(ALGORITHMS):
        xs, ys = current[key]
        reads = [y[0] for y in ys]
        writes = [y[1] for y in ys]
        faults = [y[2] for y in ys]
        plot.plot(xs, reads, '-.',c=colors[i])
        plot.plot(xs, writes, '--', c=colors[i])
        plot.plot(xs, faults, c=colors[i])
    plot.ylabel('disk reads')
    plot.xlabel('frames')
    plot.title('Reads, Writes, and Faults for {} program'.format(program))
    legends = []
    for a in ALGORITHMS:
        legends.append(a + ' read')
        legends.append(a + ' write')
        legends.append(a + ' faults')
    plot.legend(legends)
    plot.xticks(frame_counts)
    plot.show()
