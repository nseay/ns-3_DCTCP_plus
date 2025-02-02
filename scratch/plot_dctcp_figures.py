'''
Plots DCTCP Throughput Figures
'''

import argparse
from collections import defaultdict
import os
import matplotlib.pyplot as plt


def gatherData(protocol='TcpDctcp'):
  flowCompletionTimes = defaultdict(list)
  with open(os.path.join(args.dir, protocol, 'completion-times.txt'),'r') as f:
    for line in f:
      times = line.split(":")
      flowNum = float(times[0])
      completionTime = int(times[1])
      flowCompletionTimes[flowNum].append(completionTime)

  return flowCompletionTimes

def createGraph(x, y, title, xlabel, ylabel, filename, style):
  color, marker, linestyle = style
  plt.figure()
  plt.plot(x, y, c=color, marker=marker, fillstyle='none', linestyle=linestyle)
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
styles = {
  'TcpNewReno': ('green', '*', 'dashdot'),
  'TcpDctcp': ('blue', '+', 'dashed'),
  'TcpDctcpPlus': ('red', 'o', 'solid')}
if args.tcpTypeId != None:
  flowCompletionTimes = gatherData(args.tcpTypeId)
  # get average times
  flowCompletionTimes = {flowNum: sum(times) / len(times) for flowNum, times in flowCompletionTimes.items()}
  flowNums = [flowNum for flowNum in sorted(flowCompletionTimes.keys())]
  completionTimes = [flowCompletionTimes[flowNum] for flowNum in flowNums]
  
  # 1MB/ms * 8Mb/1MB * 1000ms/s
  throughputs = [1000 * 8 / time for time in completionTimes]
  filename = os.path.join(args.dir, args.tcpTypeId, 'flow-completion-times_{}-{}.png'.format(int(flowNums[0]), int(flowNums[-1])))
  createGraph(
    x=flowNums,
    y=completionTimes,
    title='Flow Completion Times',
    xlabel='Number of Flows',
    ylabel='Time (ms)',
    filename=filename,
    style=styles[args.tcpTypeId]
  )
  filename = os.path.join(args.dir, args.tcpTypeId, 'throughput_{}-{}.png'.format(int(flowNums[0]), int(flowNums[-1])))
  createGraph(
    x=flowNums,
    y=throughputs,
    title='Throughput',
    xlabel='Number of Flows',
    ylabel='Throughput (Mbps)',
    filename=filename,
    style=styles[args.tcpTypeId]
  )
else:
  # TODO: output combined graph with all protocols
  pass



