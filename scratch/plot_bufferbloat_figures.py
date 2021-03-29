'''
Plots queue occupancy, cwnd, and RTT over time for a bufferbloat experiment.

Author: Serhat Arslan (sarslan@stanford.edu)
'''

import argparse
import os
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('--dir', '-d',
                    help="Directory to find the trace files",
                    required=True,
                    action="store",
                    dest="dir")
args = parser.parse_args()

# Plot the Bottleneck Queue Occupancy over Time
qTraceTimes = []
qTraceVals = []

with open(os.path.join(args.dir, 'q.tr'),'r') as f:
    for line in f:
        qTrace = line.split()

        qTraceTimes.append(float(qTrace[0]))
        qTraceVals.append(int(qTrace[1]))

qFileName = os.path.join(args.dir, 'q.png')

plt.figure()
plt.plot(qTraceTimes, qTraceVals, c="C2")
plt.ylabel('Packets')
plt.xlabel('Seconds')
plt.grid()
plt.savefig(qFileName)
print('Saving ' + qFileName)

# Plot the Cwnd over Time
cwndTraceTimes = []
cwndTraceVals = []

with open(os.path.join(args.dir, 'cwnd.tr'),'r') as f:
    for line in f:
        cwndTrace = line.split()

        cwndTraceTimes.append(float(cwndTrace[0]))
        cwndTraceVals.append(int(cwndTrace[1])/1000) # in KB

cwndFileName = os.path.join(args.dir, 'cwnd.png')

plt.figure()
plt.plot(cwndTraceTimes, cwndTraceVals, c="C1")
plt.ylabel('cwnd (KB)')
plt.xlabel('Seconds')
plt.grid()
plt.savefig(cwndFileName)
print('Saving ' + cwndFileName)

# Plot the RTT over Time
rttTraceTimes = []
rttTraceVals = []

with open(os.path.join(args.dir, 'rtt.tr'),'r') as f:
    for line in f:
        rttTrace = line.split()

        rttTraceTimes.append(float(rttTrace[0]))
        rttTraceVals.append(int(rttTrace[1]))

rttFileName = os.path.join(args.dir, 'rtt.png')

plt.figure()
plt.plot(rttTraceTimes, rttTraceVals, c="C0")
plt.ylabel('RTT (ms)')
plt.xlabel('Seconds')
plt.grid()
plt.savefig(rttFileName)
print('Saving ' + rttFileName)
