import numpy as np
import matplotlib.pyplot as plt
import h5py

def make_4d(fname, detector_shape, bin_real, bin_det, range, rot):

    b_finish = False
    i_c = 0
    pacbed = np.zeros((512, 512))
    shape = (
        ((range[1]-range[0])//bin_real)+1,
        ((range[3]-range[2])//bin_real)+1,
        detector_shape[0]//bin_det,
        detector_shape[1]//bin_det
    )
    ds = np.zeros(shape, dtype='uint16')
    
    count = 0
    dt = np.dtype([
        ('kx', 'uint16'),
        ('ky', 'uint16'),
        ('rx', 'uint16'),
        ('ry', 'uint16')
    ])

    pre_pacbed = None
    while not b_finish:
        print(i_c)
        pre_check = np.fromfile(fname, dtype=dt,count=1,offset=int(1e7*(i_c+1))*dt.itemsize)
        if pre_check['ry'] >=range[2] or not (pre_check.size>0):

            if pre_pacbed is None:
                pre_data = np.fromfile(fname, dtype=dt,count=int(1e8),offset=int(1e7*(i_c+1))*dt.itemsize)
                pre_pacbed = np.zeros(detector_shape)
                np.add.at(pre_pacbed, (pre_data['kx'], pre_data['ky']), 1)
                shift = com(pre_pacbed)

            data = np.fromfile(fname, dtype=dt,count=int(1e7),offset=int(1e7*i_c)*dt.itemsize)
            b_within = data['ry']<range[3]
            b_finish = ~(b_within.any())
            if b_finish or not (data.size>0):
                break
            b_range = ((data['rx']>=range[0]) & (data['rx']<range[1])) & ((data['ry']>=range[2]) & (data['ry']<range[3]))
            b_seam = ((data['kx']!=255) & (data['kx']!=256)) & ((data['ky']!=255) & (data['ky']!=256))
            b_stay = b_range & b_seam

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
    return ds, pacbed, ds.sum((0,1))

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
    ricom = fft2d(comx) * fft2d(-xx) + fft2d(comy) * fft2d(-yy)
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
dir = "/mnt/B2C2DD54C2DD1E03/data/20230424/Measurement_apr_24_2023_18h48m51s/raw/"
name_fourd = 'event.dat'
name_save = None # None: generated from parameter 

# parameter
scan_shape = (2048, 2048)
detector_shape = (512, 512)
bin_real = 8
bin_det = 8
scan_crop = None # None: full range
rot = 0 # in rad
##################################################################################################

# run
if scan_crop is None:
    scan_crop = (0,scan_shape[0],0,scan_shape[1])

ds0, pacbed, pacbed_binned = make_4d(dir+name_fourd, detector_shape, bin_real, bin_det, scan_crop, rot)

if name_save is None:
    name_save = name_fourd[:-4] + '_' +\
        str(scan_shape[0]) +'x'+ str(scan_shape[1]) +'x'+ \
        str(detector_shape[0]) +'x'+ str(detector_shape[1]) +\
        '_bin' + str(bin_real) +'-'+ str(bin_det) +\
        '_crop' + str(scan_crop[0]) +'-'+ str(scan_crop[1]) +'-'+\
        str(scan_crop[2]) +'-'+ str(scan_crop[3]) +\
        '_rot' + str(rot) 

ds = np.transpose(ds0, (0,1,3,2))
ds = np.flip(ds, (0,1))
comxy = com(ds)
ricom = compute_ricom(comxy[0], comxy[1], 2)
a=10
plt.figure()
plt.imshow(ricom)
plt.show(block=False)


save_h5(ds, pacbed_binned, dir, name_save + '.h5')
save_h5(np.transpose(ds, (1,0,2,3)), pacbed_binned, dir, name_save+'_swapR.h5')

