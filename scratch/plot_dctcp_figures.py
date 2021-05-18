'''
Plots DCTCP Throughput Figures
'''

import argparse
from collections import defaultdict
import os
import matplotlib.pyplot as plt


def gatherData(protocol='TcpDctcp'):
  runsForFlow = defaultdict(list)
  with open(os.path.join(args.dir, protocol, 'completion-times.txt'),'r') as f:
    for line in f:
      times = line.split(":")

      runsForFlow[float(times[0])].append(int(times[1]))

  return runsForFlow

def createGraph(x, y, title, xlabel, ylabel, filename):
  plt.figure()
  plt.plot(x, y, c="C2")
  plt.title(title)
  plt.ylabel(ylabel)
  plt.xlabel(xlabel)
  plt.grid()
  plt.savefig(filename)
  print('Saving ' + filename)

parser = argparse.ArgumentParser()
parser.add_argument('--dir', '-d',
                    help="Directory to find the trace files",
                    required=True,
                    action="store",
                    dest="dir")
parser.add_argument('--tcpTypeId', '-t',
                    help="If specific tcpTypeId provided, graph with only this data produced.",
                    required=False,
                    action="store",
                    dest="tcpTypeId")
args = parser.parse_args()

# TODO: dctcp plus type id is a placeholder; fix later
TCP_TYPE_IDS = ['TcpNewReno', 'TcpDctcp', 'TcpDctcpPlus']

if args.tcpTypeId != None:
  runsForFlow = gatherData(args.tcpTypeId)

  flowNums = [flowNum for flowNum in runsForFlow.keys()]
  completionTimes = [sum(time) / len(time) for time in runsForFlow.values()]
  
  # 1MB/ms * 8Mb/1MB * 1000ms/s
  throughputs = [1000 * 8 / time for time in completionTimes]
  filename = os.path.join(args.dir, args.tcpTypeId + '-flow-completion-times_{}-{}.png'.format(str(flowNums[0]), str(flowNums[-1])))
  createGraph(
    x=flowNums,
    y=completionTimes,
    title='Flow Completion Times',
    xlabel='Number of Flows',
    ylabel='Time (ms)',
    filename=filename
  )
  filename = os.path.join(args.dir, args.tcpTypeId + '-throughput_{}-{}.png'.format(str(flowNums[0]), str(flowNums[-1])))
  createGraph(
    x=flowNums,
    y=throughputs,
    title='Throughput',
    xlabel='Number of Flows',
    ylabel='Throughput (Mbps)',
    filename=filename
  )
else:
  # TODO: output combined graph with all protocols
  pass



