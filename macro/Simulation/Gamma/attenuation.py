import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

def plot_efficiency_curve(csv_file, legend_name):
    # Reading data from a file into a pandas DataFrame
    data = pd.read_csv("EfficiencyCurves/"+csv_file)
    # Extracting energy, efficiency, and error columns from the DataFrame
    energy = data["Energy"].values
    efficiency = data["PhotopeakCount"].values
    error = data["Error"].values
    
    # Plotting the data
   
    
    return energy, efficiency, error

def main():
    # Initialize a figure for efficiency curves
    plt.figure(figsize=(12, 6))
    plt.subplot(1, 1, 1)  # Modify this to only plot efficiency curves

    # List of CSV files and corresponding legends
    csv_files = ["efficiency_curve_PxCT_Peek.csv","efficiency_curve_PxCT.csv"]
    legends = ["PEEK","PxCT"]
    
    # Plot each efficiency curve
    energynp, efficiencynp, errornp = plot_efficiency_curve(csv_files[0], legends[0])
    energy100, efficiency100, error100 = plot_efficiency_curve(csv_files[1], legends[1])
    thickness= 0.8255
    density = 1.32 #g/cm3
    
    #pu =1/thickness*np.log(efficiencynp[6:29]/efficiency100[6:29])
    mu = (1/thickness)*np.log(efficiencynp/efficiency100)

    plt.plot(energynp[6:29], pu)

    plt.xlabel('Energy (MeV)')
    plt.ylabel('attenuation coefficient/density (cm^2 g^-1)')
    #plt.xscale('log')
    #plt.yscale('log')
    #plt.ylim(0,0.2)
    plt.title('PxCT')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
