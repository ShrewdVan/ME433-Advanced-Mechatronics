import csv
import matplotlib.pyplot as plt 
import numpy as np 

# h_1 to h_4 are the coefficient array, which stored in another py file, prepared for FIR 
from FIR_coefficient import h_1, h_2, h_3, h_4



def MAF(address, sampling_number):

    data1 = []  # column 1, time
    data2 = []  # column 2, signal amplitude

    with open(address, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            data1.append(float(row[0]))
            data2.append(float(row[1]))

    average_data_len = len(data1)-sampling_number
    average_buffer = [0] * average_data_len
    for i in range(len(data2)-sampling_number):
        average_buffer[i] = sum(data2[i:i+sampling_number]) / sampling_number 

    maf_dt = (data1[-1] - data1[0])/average_data_len
    time = np.arange(0.0, data1[-1], maf_dt) # 10s
    maf_Fs = 1.0 / maf_dt # sample rate
    maf_Ts = 1.0 / maf_Fs; # sampling interval
    maf_ts = time
    maf_y = average_buffer # the data to make the fft from
    maf_n = len(maf_y) # length of the signal
    maf_k = np.arange(maf_n)
    maf_T = maf_n/maf_Fs
    maf_frq = maf_k/maf_T # two sides frequency range
    maf_frq = maf_frq[range(int(maf_n/2))] # one side frequency range
    maf_Y = np.fft.fft(maf_y)/maf_n # fft computing and normalization
    maf_Y = maf_Y[range(int(maf_n/2))]

    dt = (data1[-1] - data1[0])/len(data1) # step time
    Fs = 1/dt # sample rate
    Ts = 1.0/Fs; # sampling interval
    ts = data1
    y = data2 # the data to make the fft from
    n = len(y) # length of the signal
    k = np.arange(n)
    T = n/Fs
    frq = k/T # two sides frequency range
    frq = frq[range(int(n/2))] # one side frequency range
    Y = np.fft.fft(y)/n # fft computing and normalization
    Y = Y[range(int(n/2))]

    fig, (ax1, ax2) = plt.subplots(2, 1)



    ax1.set_xlabel('Time')
    ax1.set_ylabel('Amplitude')

    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')

    # Raw Data in Black
    ax1.plot(ts, y, 'k', label='Raw')
    ax2.loglog(frq, abs(Y), 'k', label='Raw')

    # MAF Data in Red
    ax1.plot(maf_ts, maf_y, 'r', label='MAF')
    ax2.loglog(maf_frq, abs(maf_Y), 'r', label=f"MAF with {sampling_number} sampling number")

    ax1.legend()
    ax2.legend()
    plt.show(block=False)
    plt.show()






def IIR(address, weight_A, weight_B):

    if weight_A + weight_B != 1:
        print("The sum of two weights need to be 1")

    data1 = []  # column 1, time
    data2 = []  # column 2, signal amplitude

    with open(address, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            data1.append(float(row[0]))
            data2.append(float(row[1]))

    previous = 0

    average_data_len = len(data1)
    average_buffer = [0] * average_data_len
    i = 0

    while i < len(data2):
        average_buffer[i] = previous * weight_A + data2[i] * weight_B
        previous = average_buffer[i]
        i += 1

    iir_dt = (data1[-1] - data1[0])/average_data_len
    time = np.arange(0.0, data1[-1], iir_dt) # 10s
    iir_Fs = 1.0 / iir_dt # sample rate
    iir_Ts = 1.0 / iir_Fs; # sampling interval
    iir_ts = time
    iir_y = average_buffer # the data to make the fft from
    iir_n = len(iir_y) # length of the signal
    iir_k = np.arange(iir_n)
    iir_T = iir_n/iir_Fs
    iir_frq = iir_k/iir_T # two sides frequency range
    iir_frq = iir_frq[range(int(iir_n/2))] # one side frequency range
    iir_Y = np.fft.fft(iir_y)/iir_n # fft computing and normalization
    iir_Y = iir_Y[range(int(iir_n/2))]

    dt = (data1[-1] - data1[0])/len(data1) # step time
    Fs = 1/dt # sample rate
    Ts = 1.0/Fs; # sampling interval
    ts = data1
    y = data2 # the data to make the fft from
    n = len(y) # length of the signal
    k = np.arange(n)
    T = n/Fs
    frq = k/T # two sides frequency range
    frq = frq[range(int(n/2))] # one side frequency range
    Y = np.fft.fft(y)/n # fft computing and normalization
    Y = Y[range(int(n/2))]

    fig, (ax1, ax2) = plt.subplots(2, 1)



    ax1.set_xlabel('Time')
    ax1.set_ylabel('Amplitude')

    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')

    # Raw Data in Black
    ax1.plot(ts, y, 'k', label='Raw')
    ax2.loglog(frq, abs(Y), 'k', label='Raw')

    # IIR Data in Red
    ax1.plot(iir_ts, iir_y, 'r', label='IIR')
    ax2.loglog(iir_frq, abs(iir_Y), 'r', label=f"IIR with A weights {weight_A} and B weights {weight_B}")

    ax1.legend()
    ax2.legend()
    plt.show()





def FIR(address, sampling_number,coef_index, method):

    data1 = []  # column 1, time
    data2 = []  # column 2, signal amplitude

    with open(address, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            data1.append(float(row[0]))
            data2.append(float(row[1]))

    average_data_len = len(data1)-sampling_number
    average_buffer = [0] * average_data_len
    for i in range(len(data2)-sampling_number):
        for j in range(len(coef_index)):
            average_buffer[i] += coef_index[j] * data2[i + j]

    fir_dt = (data1[-1] - data1[0])/average_data_len
    time = np.arange(0.0, data1[-1], fir_dt) # 10s
    fir_Fs = 1.0 / fir_dt # sample rate
    fir_Ts = 1.0 / fir_Fs; # sampling interval
    fir_ts = time
    fir_y = average_buffer # the data to make the fft from
    fir_n = len(fir_y) # length of the signal
    fir_k = np.arange(fir_n)
    fir_T = fir_n/fir_Fs
    fir_frq = fir_k/fir_T # two sides frequency range
    fir_frq = fir_frq[range(int(fir_n/2))] # one side frequency range
    fir_Y = np.fft.fft(fir_y)/fir_n # fft computing and normalization
    fir_Y = fir_Y[range(int(fir_n/2))]

    dt = (data1[-1] - data1[0])/len(data1) # step time
    Fs = 1/dt # sample rate
    Ts = 1.0/Fs; # sampling interval
    ts = data1
    y = data2 # the data to make the fft from
    n = len(y) # length of the signal
    k = np.arange(n)
    T = n/Fs
    frq = k/T # two sides frequency range
    frq = frq[range(int(n/2))] # one side frequency range
    Y = np.fft.fft(y)/n # fft computing and normalization
    Y = Y[range(int(n/2))]

    fig, (ax1, ax2) = plt.subplots(2, 1)



    ax1.set_xlabel('Time')
    ax1.set_ylabel('Amplitude')

    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')

    # Raw Data in Black
    ax1.plot(ts, y, 'k', label='Raw')
    ax2.loglog(frq, abs(Y), 'k', label='Raw')

    # FIR Data in Red
    ax1.plot(fir_ts, fir_y, 'r', label='FIR')
    ax2.loglog(fir_frq, abs(fir_Y), 'r', label=f"FIR ({method}) with {sampling_number} sampling number")

    ax1.legend()
    ax2.legend()
    plt.show(block=False)
    plt.show()


# Apply MAF to all four csv
MAF("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigA.csv",60)
MAF("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigB.csv",60)
MAF("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigC.csv",1)
MAF("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigD.csv",60)

# Apply IIR to all four csv
IIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigA.csv",0.985,0.015)
IIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigB.csv",0.985,0.015)
IIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigC.csv",0,1)
IIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigD.csv",0.95,0.05)

# Apply FIR to all four csv
FIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigA.csv", len(h_1), h_1, "Low-Pass Window")
FIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigB.csv", len(h_2), h_2, "Low-Pass Window")
FIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigC.csv", len(h_3),h_3, "Moving Average ")
FIR("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigD.csv", len(h_4),h_4, "Low-Pass Window")