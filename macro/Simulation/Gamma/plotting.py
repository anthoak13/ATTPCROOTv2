import numpy as np
import matplotlib.pyplot as plt
import pandas as pd


def plot_efficiency_curve(csv_file, legend_name):
    # Reading data from a file into a pandas DataFrame
    data = pd.read_csv(csv_file)
    # Extracting energy, efficiency, and error columns from the DataFrame
    energy = data["Energy"].values
    efficiency = data["Efficiency"].values
    error = data["Error"].values
    
    # Plotting the data
    plt.errorbar(energy, efficiency, yerr=error, fmt='.', label=f'{legend_name} Data')
    
    return energy, efficiency, error

def main():
    # Initialize a figure for efficiency curves
    plt.figure(figsize=(12, 6))
    plt.subplot(1, 1, 1)  # Modify this to only plot efficiency curves

    # List of CSV files and corresponding legends
    csv_files = ["efficiency_curve_PxCT.csv","efficiency_curve_PxCT_Peek.csv", "efficiency_curve_PxCT_PVC.csv"]
    legends = ["PxCT","PEEK","PVC"]
    
    # Plot each efficiency curve
    for csv_file, legend in zip(csv_files, legends):
        plot_efficiency_curve("EfficiencyCurves/"+csv_file, legend)

    plt.xlabel('Energy (MeV)')
    plt.ylabel('Efficiency (%)')
    plt.title('Efficiency Curves')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()

