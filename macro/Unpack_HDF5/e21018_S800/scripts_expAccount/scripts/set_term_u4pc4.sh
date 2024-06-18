#!/bin/bash

gnome-terminal --title="copy"
gnome-terminal --title="ATTPC unpack" -- ssh -qY e21018@n103.compute.nscl.msu.edu
gnome-terminal --title="S800 unpack" -- ssh -qY e21018@expanalysis004
