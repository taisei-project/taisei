#!/usr/bin/env python3

# This is a quick testing script for taisei’s random number generator.
# To use it, compile taisei with -DTSRAND_FLOATTEST and run it once.
#
# The script will plot a histogram of the data, that should be a
# uniform distribution from 0 to 1.
#
# The error bars are sqrt(bincount) according to poisson statistics.
# This is okay at large sample sizes.
#
# Lastly, we it does a χ²-test. For large sample sizes, (binheight-1)/err
# follows a Gaussian distribution. So the sum over this squared will follow the
# χ²-distribution with degrees of freedom = number of bins - 1.
#
# For this distribution, χ²/dof should be around 1.

import numpy as np
import matplotlib.pyplot as plt
import scipy.stats

plt.style.use('bmh')

d = np.genfromtxt("/tmp/rand_test")
edges = np.arange(-0.1,1.1,0.05)
binheight,edges = np.histogram(d,bins=edges)
err = np.sqrt(binheight)
width = edges[1]-edges[0]

norm = len(d)*width
binheight = binheight/norm
err = err/norm

plt.figure(figsize=(7,5))
plt.bar(edges[:-1],binheight,yerr=err,color="#2255bb",width=width)

# only consider non-empty bins
err = err[binheight != 0]
binheight = binheight[binheight != 0]
dof = len(binheight)-1

chisq, p = scipy.stats.chisquare(binheight*norm, f_exp=norm, ddof=0, axis=0)
print("χ²/dof: %g\np-value: %g"%( chisq/dof, p))

plt.title("N = %d, $\chi^2/dof$ = %g"%(len(d),chisq/dof))
plt.xlim(-0.1,1.1)
plt.savefig("/tmp/hist.png",dpi=200)
plt.show()
