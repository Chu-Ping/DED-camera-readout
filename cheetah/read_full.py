import numpy as np
import matplotlib.pyplot as plt
import h5py

fname = '' # file address
read_begin = 0
read_count = 1e5

dt = np.dtype([
    ('kx', 'uint16'),
    ('ky', 'uint16'),
    ('rx', 'uint16'),
    ('ry', 'uint16'),
    ('toa', 'uint64'), # unit: 25/16 ns
    ('tot', 'uint64')  # unit: 25 ns
])
data = np.fromfile(fname, dtype=dt,count=read_count*dt.itemsize,offset=read_begin)

