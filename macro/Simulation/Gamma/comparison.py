import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def load_and_prepare_data(csv_file):
    # Reading data from a file into a pandas DataFrame
    data = pd.read_csv("EfficiencyCurves/"+csv_file)
    # Assuming Energy and Efficiency columns exist
    energy = data['Energy'].values
    efficiency = data['Efficiency'].values
    return energy, efficiency

def calculate_and_plot_differences(csv_files, legends):
    # Load data from each CSV file
    datasets = [load_and_prepare_data(file) for file in csv_files]

    # Initialize plotting
    plt.figure(figsize=(12, 6))
    
    # Calculate and plot the percentage differences
    energy1, efficiency1 = datasets[0]
    energy2, efficiency2 = datasets[1]
    len(efficiency1)
    len(efficiency2)
    abs_diff = np.abs(efficiency1 - efficiency2)
    percent_diff = (abs_diff / efficiency1) * 100
    
    plt.plot(energy1, percent_diff, label=f'% diff between {legends[0]} and {legends[1]}')
    
    plt.xlabel('Energy (MeV)')
    plt.ylabel('Percentage Difference (%)')
    plt.title('Difference Between Efficiency Curves')
    plt.legend()
    plt.grid(True)
    plt.show()
    plt.ylim(0,20)

def main():
    # Specify CSV files and legends
    csv_files = ["efficiency_curve_PxCT_Peek.csv","efficiency_curve_PxCT.csv"]
    legends = ["PEEK","PxCT"]
    
    
    calculate_and_plot_differences(csv_files, legends)

if __name__ == "__main__":
    main()
