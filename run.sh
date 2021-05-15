#!/bin/bash
[ ! -d outputs ] && mkdir outputs

NUMFLOWS=20
for TCPTYPE in "TcpDctcp"; do
    DIR=outputs/
    [ ! -d $DIR ] && mkdir $DIR

    # Run the NS-3 Simulation
    ./waf --run "scratch/scratch-simulator --outputFilePath=$DIR --numFlows=$NUMFLOWS --enableSwitchEcn=$true"
    # Plot the trace figures
    # python3 scratch/plot_bufferbloat_figures.py --dir $DIR/
done

echo "Simulations are done! Results can be retrieved via the server"
# python3 -m http.server
