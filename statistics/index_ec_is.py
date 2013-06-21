#!/bin/python
import fileinput
import matplotlib.pyplot as plt
import numpy as np

element_counts = []
index_sizes = []

for line in fileinput.input():
	v = line.split()
	element_counts.append(int(v[0]))
	index_sizes.append(int(v[1]))

fig = plt.figure()

ax = fig.add_subplot(111)
ax.set_xlabel(u'element count')
ax.set_ylabel(u'index size')

#ax.grid(True, linestyle='-', color='0.75')

#plt.plot(element_counts, index_sizes, 'ro')

#for i in range(0, len(element_counts)):
#	ax.scatter([element_counts[i]], [index_sizes[i]])

#heatmap, xedges, yedges = np.histogram2d(element_counts, index_sizes, bins=(100,100))
#extent = [xedges[0], xedges[-1], yedges[0], yedges[-1]]

ax.hist(index_sizes, 10000, color='green')
ax.set_xscale('log', basex=2)
ax.set_yscale('symlog')
#plt.clf()
#plt.imshow(heatmap, extent=extent)
plt.show()

