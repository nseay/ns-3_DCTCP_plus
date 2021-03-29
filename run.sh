#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=60
BWNET=10
# TODO: If you want the Base RTT to be 40ms what should the delay on each
# link be?  Set this value correctly.
DELAY=__FILL_IN_HERE__

for QSIZE in 20 100; do
    DIR=outputs/bb-q$QSIZE
    [ ! -d $DIR ] && mkdir $DIR

    # Run the NS-3 Simulation
    ./waf --run "scratch/bufferbloat --bwNet=$BWNET --delay=$DELAY --time=$TIME --maxQ=$QSIZE"
    # Plot the trace figures
    python3 scratch/plot_bufferbloat_figures.py --dir $DIR/
done

echo "Simulations are done! Results can be retrieved via the server"
python3 -m http.server
