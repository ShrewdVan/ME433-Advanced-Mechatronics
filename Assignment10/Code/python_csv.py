import csv
import matplotlib.pyplot as plt # for plotting
import numpy as np # for sine function

data1 = []  # column 1, time
data2 = []  # column 2, signal

with open("/Users/raidriarpersonal/Documents/GitHub/ME433-Advanced-Mechatronics/Assignment10/Code/sigA.csv", "r") as f:
    reader = csv.reader(f)
    for row in reader:
        data1.append(float(row[0]))
        data2.append(float(row[1]))


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
ax1.plot(ts,y,'b')
ax1.set_xlabel('Time')
ax1.set_ylabel('Amplitude')
ax2.loglog(frq,abs(Y),'b') # plotting the fft
ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')
plt.show()

print(f"Sampling Rate is {Fs}")
# plt.plot(data1,data2,'b-*')
# plt.xlabel('Time [s]')
# plt.ylabel('Signal')
# plt.title('Signal vs Time')
# plt.show()
