# %%
import os
import numpy as np
import dask.array as da
import matplotlib.pyplot as plt
import h5py
from dask.diagnostics import ProgressBar
ProgressBar().register()

def fft2d(arr, b_norm=False):
    out = np.fft.fftshift(np.fft.fft2(np.fft.ifftshift(arr)))
    if b_norm:
        out /= arr.shape[0]**0.5 * arr.shape[1]**0.5
    return out

def ifft2d(arr, b_norm=False):
    out =  np.fft.fftshift(np.fft.ifft2(np.fft.ifftshift(arr)))
    if b_norm:
        out *= arr.shape[0]**0.5 * arr.shape[1]**0.5
    return out

def safe_div(a,b):
    if b.shape:
        b[b==0] = 1
    return a/b

def com(data):
    ramp = np.arange(data.shape[-1])
    dose = data.sum((-2,-1))
    com = [
        safe_div((data*ramp[:,np.newaxis]).sum((-2,-1)), dose) - data.shape[-2]//2,
        safe_div((data*ramp).sum((-2,-1)), dose) - data.shape[-1]//2
    ]
    if len(data.shape)==4:
        com[0][dose==0] = 0
        com[1][dose==0] = 0    
    return com

def compute_ricom(comx, comy, kernel_size):
    template = np.zeros(comx.shape)
    x = np.linspace(-0.5, 0.5, kernel_size*2+1, endpoint=True)
    _xx, _yy = np.meshgrid(x,x)
    xx = np.copy(template)
    yy = np.copy(template)
    xx[ xx.shape[0]//2-kernel_size:xx.shape[0]//2+kernel_size+1,
        xx.shape[1]//2-kernel_size:xx.shape[1]//2+kernel_size+1
        ] = _xx
    yy[ yy.shape[0]//2-kernel_size:yy.shape[0]//2+kernel_size+1,
        yy.shape[1]//2-kernel_size:yy.shape[1]//2+kernel_size+1
        ] = _yy
    zz = xx**2 + yy**2
    xx /= zz
    yy /= zz
    xx[zz==0] = 0
    yy[zz==0] = 0
    ricom = fft2d(comx) * fft2d(-xx) + fft2d(comy) * fft2d(-yy)
    ricom = np.real(ifft2d(ricom))
    return ricom

def save_h5(ds, pacbed, dir, name):
    f = h5py.File(dir+'/'+name, 'w')
    f.create_dataset("ds", data=ds, dtype='uint8')
    f.create_dataset("probe", data=pacbed, dtype='single')
    f.close()
    print('saved')


class MIB():
    def __init__(self, filename, depth, raw=True):
        self.filename = filename
        self.depth = depth
        self._make_dtype()
        self.raw = raw

    def _mmap(self, n_data):
        # common_header_size = 384
        return np.memmap(
            self.filename, dtype=self.dtype, 
            # mode='r', offset=common_header_size, 
            mode='r', offset=0, 
            shape = n_data
            )
    
    def _make_dtype(self):
        header_size = 384
        header_type = np.string_
        if self.depth == 1:
            data_size = int(256**2/8)
            data_type = 'uint8'
        elif self.depth == 6:
            data_size = int(256**2)
            data_type = 'uint8'
        elif self.depth == 12:
            data_size = int(256**2)
            data_type = 'uint16'
        self.dtype = np.dtype([
            ('header', header_type, header_size), 
            ('frame', data_type, data_size)])
        # self.dtype = np.dtype([('frame', data_type, data_size)])
    
    def _reshape(self, array, scan_shape, scan_crop):
        if self.depth == 1 and self.raw:
            array_raw = da.reshape(
                array, (scan_shape[0], scan_shape[1], 256, 32))
            array = da.zeros((scan_shape[0], scan_shape[1], 256, 256),'uint16')
            for n in range(8):
                nn = 7-n
                array[..., n::8] = array_raw%(2**(nn+1))//(2**nn)
        elif self.depth == 1 and not self.raw:
            array = da.reshape(
                array, (scan_shape[0], scan_shape[1], 256, 256))
        elif self.depth == 6:
            array = da.reshape(
                array, (scan_shape[0], scan_shape[1], 256, 256))
        elif self.depth == 12:
            array = da.reshape(
                array, (scan_shape[0], scan_shape[1], 256, 256))
        return array[scan_crop[0]:scan_crop[1], scan_crop[2]:scan_crop[3]]
    
    def _swap_column(self, array):
        if self.raw:
            if self.depth == 1:
                order = [63 + 64*i -ii for i in range(4) for ii in range(64)]
            elif self.depth == 6:
                order = [7 + 8*i -ii for i in range(32) for ii in range(8)]
            elif self.depth == 12:
                order = [3 + 4*i -ii for i in range(64) for ii in range(4)]
            return array[...,order]
        else:
            return array
    
    def _center(self, array):
        shift = np.round(com(array.sum((0,1)))).astype('int')
        if shift[0]>0:
            array[...,:-shift[0],:] = array[...,shift[0]:,:]
            array[...,-shift[0]:,:] = 0
        elif shift[0]<0:
            array[...,-shift[0]:,:] = array[...,:shift[0],:]
            array[...,:-shift[0],:] = 0
        if shift[1]>0:
            array[...,:-shift[1]] = array[...,shift[1]:]
            array[...,-shift[1]:] = 0
        elif shift[1]<0:
            array[...,-shift[1]:] = array[...,:shift[1]]
            array[...,:-shift[1]] = 0
        return array

    def _rotate(self, array, rot, precision):
        rot = np.angle(np.exp(1j*rot))
        if rot % (np.pi/2) == 0: # no needs for sparse operation
            if rot == np.pi/2:
                array = da.transpose(array, (0,1,3,2))[...,::-1]
            elif (rot == np.pi) | (rot == -np.pi):
                array = array[...,::-1,::-1]
            elif rot == -np.pi/2:
                array = da.transpose(array, (0,1,3,2))[...,::-1,:]
        # else: # this rotation seems to take aweful long time
        return array

    def _bin(self, array, bin_real, bin_det):
        array =  da.reshape(array, (
            array.shape[0] // bin_real,bin_real, 
            array.shape[1] // bin_real,bin_real, 
            array.shape[2] // bin_det,bin_det, 
            array.shape[3] // bin_det,bin_det
            )).sum((1,3,5,7))
        return array.compute()

    def load_as_dask(self, scan_shape):
        array = da.from_array(
            self._mmap(scan_shape[0]*scan_shape[1])
            )
        return array['frame']
    
    def make_4d(self, dask_array, scan_shape, 
                bin_real, bin_det, scan_crop, 
                rot, precision="low"):
        dask_array = self._reshape(dask_array, scan_shape, scan_crop)
        dask_array = self._swap_column(dask_array)
        dask_array = self._rotate(dask_array, rot, precision)
        array = self._bin(dask_array, bin_real, bin_det)
        array = self._center(array)
        return array, array.sum((0,1))

            
# %%
######################################## USER INPUT ##############################################
# files
dir = "/mnt/B2C2DD54C2DD1E03/data/linux/20221017_au-pentatwin/"
name_mib = '6.mib'
name_save = None # None: generated from parameter 

# parameter
scan_shape = (256, 257)
detector_shape = (256, 256)
bin_real = 1
bin_det = 4
scan_crop = (0,100,0,100) # None: full range
rot = 0 # in rad
depth = 6 # 1, 6, 12; bit depth of the mib file
##################################################################################################
if scan_crop is None:
    scan_crop = (0,scan_shape[0],0,scan_shape[1])

if name_save is None:
    name_save = name_mib[:-4] + \
        '_' + str(scan_shape[0]) +'x'+ str(scan_shape[1]) +'x'+ \
        str(detector_shape[0]) +'x'+ str(detector_shape[1]) +\
        '_bin' + str(bin_real) +'-'+ str(bin_det) +\
        '_crop' + str(scan_crop[0]) +'-'+ str(scan_crop[1]) +'-'+\
        str(scan_crop[2]) +'-'+ str(scan_crop[3]) +\
        '_rot' + str(rot) 

mib = MIB(dir+name_mib, depth, raw=False)
dask_array = mib.load_as_dask(scan_shape)
ds, pacbed_binned = mib.make_4d(dask_array, scan_shape, bin_real, 
                    bin_det, scan_crop, rot)

# adf_det = np.ones((64,64))
# adf_det[pacbed_binned>(pacbed_binned.max()*0.1)] = 0
# adf = (ds*adf_det).sum((2,3))
# plt.imshow(adf)
# plt.show()

save_h5(ds, pacbed_binned, dir, name_save + '.h5')
