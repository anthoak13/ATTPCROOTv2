import sklearn.cluster as skcluster
import numpy as np
import os

vmcworkdir = os.getenv('VMCWORKDIR')
print(vmcworkdir)
min_size = 5
min_points = 5
cluster_selection_epsilon = 0.5

clusterizer = skcluster.HDBSCAN(  # type: ignore
        min_cluster_size=min_size,
        min_samples=min_points,
        allow_single_cluster=True,
        cluster_selection_epsilon=cluster_selection_epsilon,
    )

file_path = 'HDBSCANDataset/event_1.dat'  # Replace with your actual file path

data = np.loadtxt(file_path, delimiter=' ', skiprows=1)
cluster_data = np.empty(shape=(len(data),3))
cluster_data[:,:] = data[:,:3]
clusterizer.fit(cluster_data)
