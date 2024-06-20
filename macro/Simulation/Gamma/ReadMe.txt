Hi!
Welcome to the Gamma scripts! This is a collection of scripts that I have written to help me with my work. I hope you find them useful too.
Author: Ari Andalib 
Email: AriAndalib@gmail.com
"""
# Folders
`data` - Holds all the gamma simulation information
`EfficiencyCurves` - Holds generated efficiency curves data.

# scripts

`attenuation.py' - finds the attentuation coefficient using two different Efficiency Curves one with an without the material.
`comparison.py` - plots the absolute percentage diffrence between Efficiency Curves.
`Efficiency_curve.py` - Runs simulations for a variety of energy levels and calculates efficiency curve csvs.
`Gamma_sim_isotope.C` - Gamma simulations of some common gamma calibration sources.
`gamma_sim.C` - The gamma simulation.
`plotting.py` - plots efficiency curves.
`Simp_gamma_analysis.C` - information such as hits and efficency of the simulation is produced with no spectrum
`Spectrum.C` - shows spectrum of generated gamma simulation.
`Trajectory.C` - shows the gamma detector interactions for the events simulated.

# Variables of interest

## `attenuation.py'
- csv_files : first csv file if efficency curve with the material second is without.
- thickness: thickness of material in cm.
- density: density of material in g/cm^3.

## `Efficiency_curve.py`
- energy_values : the energy values in MeV that will be simulated.
- file_path : location and name of saved csv file.

## `gamma_sim.C`
- Energy: in MeV of Simulated gamma ray.
- Event_no: number of gamma rays produced.
*the other functions can be found in the header defnition of AtGammaDummyGenerator.

# Note
- for most of the .C files make sure that the geometry used, inputs and outputs are detector specific.
