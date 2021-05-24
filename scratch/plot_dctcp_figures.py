'''
Plots DCTCP Throughput Figures
'''

import argparse
from collections import defaultdict
import os
import matplotlib.pyplot as plt

TCP_TYPE_IDS = ['TcpNewReno', 'TcpDctcp', 'TcpDctcpPlus']
STYLES = {
  'TcpNewReno': ('green', '*', 'dashdot'),
  'TcpDctcp': ('blue', '+', 'dashed'),
  'TcpDctcpPlus': ('red', 'o', 'solid')
}

def gatherData(protocol='TcpDctcp'):
  flowCompletionTimes = defaultdict(list)
  with open(os.path.join(args.dir, protocol, 'completion-times.txt'),'r') as f:
    for line in f:
      times = line.split(":")
      flowNum = float(times[0])
      completionTime = int(times[1])
      flowCompletionTimes[flowNum].append(completionTime)

  return flowCompletionTimes

def createGraph(x, y, title, xlabel, ylabel, filename, prots):
  if prots[0] is None:
    prots = TCP_TYPE_IDS
  plt.figure()
  for i in range(len(x)):
    color, marker, linestyle = STYLES[prots[i]]
    plt.plot(x[i], y[i], c=color, marker=marker, fillstyle='none', linestyle=linestyle)
  plt.title(title)
  plt.ylabel(ylabel)
  plt.xlabel(xlabel)
  if len(x) == 3:
    plt.legend(TCP_TYPE_IDS)
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



if args.tcpTypeId != None:
  flowCompletionTimes = [gatherData(args.tcpTypeId)]
else:
  flowCompletionTimes = [gatherData(typeId) for typeId in TCP_TYPE_IDS]
# get average times
flowCompletionTimes = [{flowNum: sum(times) / len(times) for flowNum, times in fct.items()} for fct in flowCompletionTimes]
flowNums = [[flowNum for flowNum in sorted(fct.keys())] for fct in flowCompletionTimes]
completionTimes = [[flowCompletionTimes[i][flowNum] for flowNum in fn] for i, fn in enumerate(flowNums)]

# 1MB/ms * 8Mb/1MB * 1000ms/s
throughputs = [[1000 * 8 / time for time in ct] for ct in completionTimes]
if args.tcpTypeId is not None:
  filename = os.path.join(args.dir, args.tcpTypeId, 'flow-completion-times_{}-{}.png'.format(int(flowNums[0][0]), int(flowNums[0][-1])))
else:
  filename = os.path.join(args.dir, 'flow-completion-times_{}-{}.png'.format(int(flowNums[0][0]), int(flowNums[0][-1])))

createGraph(
  x=flowNums,
  y=completionTimes,
  title='Flow Completion Times {}'.format('' if args.tcpTypeId is None else args.tcpTypeId),
  xlabel='Number of Flows',
  ylabel='Time (ms)',
  filename=filename,
  prots=[args.tcpTypeId]
)
# filename = os.path.join(args.dir, args.tcpTypeId, 'throughput_{}-{}.png'.format(int(flowNums[0]), int(flowNums[-1])))
filename = filename.replace('flow-completion-times', 'throughput')
createGraph(
  x=flowNums,
  y=throughputs,
  title='Throughput {}'.format('' if args.tcpTypeId is None else args.tcpTypeId),
  xlabel='Number of Flows',
  ylabel='Throughput (Mbps)',
  filename=filename,
  prots=[args.tcpTypeId]
)


