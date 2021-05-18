#!/bin/bash
BASEDIR=outputs
[ ! -d $BASEDIR ] && mkdir $BASEDIR

PRINTLASTXBYTESRECIEVED=0
# PRINTLASTXBYTESRECIEVED=10000
declare -a Types=("TcpDctcp" "TcpNewReno")
for TCPTYPE in ${Types[@]}; do
  DIR=outputs/$TCPTYPE/
  [ ! -d $DIR ] && mkdir $DIR
  if [ "$PRINTLASTXBYTESRECIEVED" = 0 ]
  then
    rm "${DIR}completion-times.txt"
  else
    rm "${DIR}completion-times-"*flows.txt
  fi

  NUMSENDERS=9
  # for NUMFLOWS in {1..100}; do
  for NUMFLOWS in {1..50}; do
    for RUN in {1..100}; do
      # NUMSENDERS=$NUMFLOWS
      if [ "$PRINTLASTXBYTESRECIEVED" = 0 ]
      then
        FILENAME="completion-times.txt"
      else
        FILENAME="completion-times-${NUMFLOWS}flows.txt"
      fi
      # Run the NS-3 Simulation
      NS_GLOBAL_VALUE="RngRun=$RUN" ./waf --run "scratch/scratch-simulator --outputFilePath=$DIR --tcpTypeId=$TCPTYPE --outputFilename=$FILENAME --numFlows=$NUMFLOWS --numSenders=$NUMSENDERS --enableSwitchEcn=true --printLastXBytesReceived=$PRINTLASTXBYTESRECIEVED"
    done
  done
  # Plot the trace figures
  if [ "$PRINTLASTXBYTESRECIEVED" = 0 ]
    then
      python3 scratch/plot_dctcp_figures.py --dir $BASEDIR --tcpTypeId $TCPTYPE
  fi
done

echo "Simulations are done!"
# python3 -m http.server
