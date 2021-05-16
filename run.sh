#!/bin/bash
BASEDIR=outputs
[ ! -d $BASEDIR ] && mkdir $BASEDIR

PRINTLASTXBYTESRECIEVED=0
# PRINTLASTXBYTESRECIEVED=10000

for TCPTYPE in "TcpDctcp"; do
  DIR=outputs/$TCPTYPE/
  [ ! -d $DIR ] && mkdir $DIR
  if [ "$PRINTLASTXBYTESRECIEVED" = 0 ]
  then
    rm "${DIR}completion-times.txt"
  else
    rm "${DIR}completion-times-"*flows.txt
  fi

  for NUMFLOWS in {1..50}; do

      if [ "$PRINTLASTXBYTESRECIEVED" = 0 ]
      then
        FILENAME="completion-times.txt"
      else
        FILENAME="completion-times-${NUMFLOWS}flows.txt"
      fi
      # Run the NS-3 Simulation
      ./waf --run "scratch/scratch-simulator --outputFilePath=$DIR --outputFilename=$FILENAME --numFlows=$NUMFLOWS --enableSwitchEcn=true --printLastXBytesReceived=$PRINTLASTXBYTESRECIEVED"
    done
    # Plot the trace figures
    if [ "$PRINTLASTXBYTESRECIEVED" = 0 ]
      then
        python3 scratch/plot_dctcp_figures.py --dir $BASEDIR --tcpTypeId $TCPTYPE
    fi
done

echo "Simulations are done!"
# python3 -m http.server
