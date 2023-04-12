import numpy as np
import dask
import dask.array as da
import h5py
import matplotlib.pyplot as plt


def make_4d(fname, dwell_time, line_shape, detector_shape, bin_real, bin_det, range, rot):
    b_finish = False
    i_c = 0
    pacbed = np.zeros(detector_shape)
    shape = (
        ((range[1]-range[0])//bin_real)+1,
        ((range[3]-range[2])//bin_real)+1,
        detector_shape[0]//bin_det,
        detector_shape[1]//bin_det
    )
    ds = np.zeros(shape, dtype='uint16')
    
    count = 0
    dt = np.dtype([
        ('index', np.uint32), 
        ('toa', np.uint64), 
        ('overflow', np.uint8),
        ('ftoa', np.uint8), 
        ('tot', np.uint16)
    ])

    pre_pacbed = None
    while not b_finish:
        print(i_c)
        pre_check = np.fromfile(fname, dtype=dt,count=1,offset=int(1e7*(i_c+1))*dt.itemsize)
        if (pre_check['toa']*25//dwell_time//line_shape >=range[2]) or not (pre_check.size>0):
            if pre_pacbed is None:
                pre_data = np.fromfile(fname, dtype=dt,count=int(1e8),offset=int(1e7*(i_c+1))*dt.itemsize)
                pre_pacbed = np.zeros(detector_shape)
                np.add.at(pre_pacbed, (pre_data['index']%256, pre_data['index']//256), 1)
                shift = com(pre_pacbed)

            d = np.fromfile(fname, dtype=dt,count=int(1e7),offset=int(1e7*i_c)*dt.itemsize)
            data = {
                'kx': d['index']%256,
                'ky': d['index']//256,
                'rx': d['toa']*25//dwell_time%line_shape,
                'ry': d['toa']*25//dwell_time//line_shape,
            }
            del d
            b_finish = (data['ry']>=range[3] ).all()
            if b_finish or not (data.size>0):
                break
            b_stay = ((data['rx']>=range[0]) & (data['rx']<range[1])) & ((data['ry']>=range[2]) & (data['ry']<range[3]))

            rx = data['rx'][b_stay] - range[0]
            ry = data['ry'][b_stay] - range[2]
            kx = data['kx'][b_stay]
            ky = data['ky'][b_stay]

            kx, ky = [
                ( kx-shift[0]-detector_shape[0]//2 ) * np.cos(rot) - 
                ( ky-shift[1]-detector_shape[1]//2 ) * np.sin(rot) + detector_shape[0]//2,
                ( ky-shift[1]-detector_shape[1]//2 ) * np.cos(rot) + 
                ( kx-shift[0]-detector_shape[0]//2 ) * np.sin(rot) + detector_shape[1]//2
                ]

            b_stay = \
                (kx>=0).astype('uint8') +\
                (kx<(detector_shape[0])).astype('uint8') +\
                (ky>=0).astype('uint8') +\
                (ky<(detector_shape[1])).astype('uint8')
            b_stay = b_stay==4

            rx = rx[b_stay].astype('uint16')
            ry = ry[b_stay].astype('uint16')
            kx = kx[b_stay].astype('uint16')
            ky = ky[b_stay].astype('uint16')

            _count = b_stay.sum()
            np.add.at(ds, (
                rx//bin_real,
                ry//bin_real,
                kx//bin_det,
                ky//bin_det,
                ), 1)
            np.add.at(pacbed, (kx, ky), 1)
            count += _count
        i_c += 1
    return ds[:-1,:-1], pacbed, ds.sum((0,1))

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
    comx = safe_div((data.sum(-1)*ramp).sum(-1),dose) - data.shape[-2]//2
    comy = safe_div((data.sum(-2)*ramp).sum(-1),dose) - data.shape[-1]//2
    com = [comx, comy]
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
    ricom = fft2d(comx) * fft2d(xx) + fft2d(comy) * fft2d(yy)
    ricom = np.real(ifft2d(ricom))
    return ricom

def save_h5(ds, pacbed, dir, name):
    f = h5py.File(dir+'/'+name, 'w')
    f.create_dataset("ds", data=ds, dtype='uint8')
    f.create_dataset("probe", data=pacbed, dtype='single')
    f.close()
    print('saved')


######################################## USER INPUT ##############################################
# files
dir = '/mnt/B2C2DD54C2DD1E03/data/linux/data_result/Nadine/FAPbBr3 NCs different parameters/13mrad good data set/'
name_t3p = 'Tue_Oct_25_20_14_24_2022_STEM_200kV_2048x2048_1_us_3_scans.t3p'
name_save = None # None: generated from parameter 

# parameter
scan_shape = (2049, 2048)
detector_shape = (256, 256)
dwell_time = 1000
bin_real = 2
bin_det = 4
scan_crop = (0,1000,1000,2000) # None: full range
rot = 0 # in rad
##################################################################################################

if scan_crop is None:
    scan_crop = (0,scan_shape[0],0,scan_shape[1])
ds, pacbed, pacbed_binned = make_4d(dir+name_t3p, dwell_time, scan_shape[0], detector_shape, bin_real, bin_det, scan_crop, rot)

comxy = com(ds)
ricom = compute_ricom(comxy[0], comxy[1], 5)

if name_save is None:
    name_save = name_t3p[:-4] + '_' +\
        str(scan_shape[0]) +'x'+ str(scan_shape[1]) +'x'+ \
        str(detector_shape[0]) +'x'+ str(detector_shape[1]) +\
        '_bin' + str(bin_real) +'-'+ str(bin_det) +\
        '_crop' + str(scan_crop[0]) +'-'+ str(scan_crop[1]) +'-'+\
        str(scan_crop[2]) +'-'+ str(scan_crop[3]) +\
        '_rot' + str(rot) 

    
save_h5(ds, pacbed_binned, dir, name_save + '.h5')
save_h5(np.transpose(ds, (1,0,3,2)), np.transpose(pacbed_binned,(1,0)), dir, name_save+'_swap.h5')

