import numpy as np
import matplotlib.pyplot as plt

fig, ax = plt.subplots()

f = file('stats', 'r')

lines = f.readlines()

testnames = eval(lines[0])
without_baggy = eval(lines[1])
with_baggy = eval(lines[2])

f.close()

n_groups = len(testnames)
index = np.arange(n_groups)
bar_width = 0.35

opacity = 0.4
error_config = {'ecolor': '0.3'}

rects1 = plt.bar(index, with_baggy, bar_width,
                 alpha=opacity,
                 color='b',
                 yerr=None,
                 error_kw=error_config,
                 label='binary allocator with baggy')

rects2 = plt.bar(index + bar_width, without_baggy, bar_width,
                 alpha=opacity,
                 color='r',
                 yerr=None,
                 error_kw=error_config,
                 label='binary allocator without baggy')

plt.xlabel('Test')
plt.ylabel('Performance')
plt.title('Performance normalized by LLVM with standard allocator')
plt.xticks(index + bar_width, testnames)
plt.legend()

plt.tight_layout()
plt.show()