'''
Plots DCTCP Throughput Figures
'''

import argparse
import os
import matplotlib.pyplot as plt


def gatherTcpOrDctcpData(protocol='TcpDctcp'):
  flowNums=[]
  completionTimes=[]
  with open(os.path.join(args.dir, protocol, 'completion-times.txt'),'r') as f:
    for line in f:
      times = line.split(":")

      flowNums.append(float(times[0]))
      completionTimes.append(int(times[1]))

  return flowNums, completionTimes

def gatherDctcpPlusData():
  '''
  DCTCP+ incorporates randomness, so many experiments are run
  and values must be averaged to get single datapoint for graph
  which is analogous to the datapoints of the other protocols
  '''
  # TODO
  pass

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

# TODO: tcp and dctcp plus type ids are placeholders; fix later
TCP_TYPE_IDS = ['TCP-Reno', 'TcpDctcp', 'TcpDctcpPlus']

if args.tcpTypeId != None:
  if args.tcpTypeId != 'TcpDctcpPlus':
    flowNums, completionTimes = gatherTcpOrDctcpData(args.tcpTypeId)
  else:
    flowNums, completionTimes = gatherDctcpPlusData(args.tcpTypeId)
  
  # 1MB/ms * 8Mb/1MB * 1000ms/s
  throughputs = [1000 * 8 / time for time in completionTimes]
  filename = os.path.join(args.dir, args.tcpTypeId + '-throughput_{}-{}.png'.format(str(flowNums[0]), str(flowNums[-1])))
  plt.figure()
  plt.plot(flowNums, throughputs, c="C2")
  plt.title('Throughput')
  plt.ylabel('Throughput (Mbps)')
  plt.xlabel('Number of Flows')
  plt.grid()
  plt.savefig(filename)
  print('Saving ' + filename)
else:
  # TODO: output combined graph with all protocols
  pass



