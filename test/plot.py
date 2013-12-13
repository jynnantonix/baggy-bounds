import numpy as np
import matplotlib.pyplot as plt

fig, ax = plt.subplots()

f = file('stats', 'r')

lines = f.readlines()

testnames = eval(lines[0])
without_baggy = eval(lines[1])
with_baggy = eval(lines[2])
optimized = eval(lines[3])

f.close()

n_groups = len(testnames)
index = np.arange(n_groups)
bar_width = 0.30

opacity = 0.4
error_config = {'ecolor': '0.3'}

rects2 = plt.bar(index, without_baggy, bar_width,
                 alpha=opacity,
                 color='r',
                 yerr=None,
                 error_kw=error_config,
                 label='buddy allocator without baggy')

rects1 = plt.bar(index + bar_width, with_baggy, bar_width,
                 alpha=opacity,
                 color='b',
                 yerr=None,
                 error_kw=error_config,
                 label='buddy allocator with baggy')


rects3 = plt.bar(index + bar_width * 2, optimized, bar_width,
                 alpha=opacity,
                 color='g',
                 yerr=None,
                 error_kw=error_config,
                 label='optimized')

plt.xlabel('')
plt.ylabel('Performance')
plt.title('')
plt.xticks(index + bar_width, testnames)
plt.legend()

plt.tight_layout()
plt.show()
