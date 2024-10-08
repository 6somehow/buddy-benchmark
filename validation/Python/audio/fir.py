#    fir.py - FIR filter class for audio validation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# This File is for testing of the FIR filter in buddy-mlir.

from cffi import FFI
import matplotlib.pyplot as plt
import scipy as sp
import numpy as np
import os
import sys
import time
import math
import random
from .audio_test import AudioTest
import utils.audio_format as af
import utils.lib_path as lp


class FIRTest(AudioTest):
    """FIR test class.
         Test params:
            - file: path to the audio file to be filtered.
            - fconf: filter configurations.
            - lib: path to the library file.

    """
    default_param = {"file": "../../benchmarks/AudioProcessing/Audios/NASA_Mars.wav",
                     "fconf": ('kaiser', 4.0),
                     "lib": "../../build/validation/AudioProcessing/libAudioValidationLib"}

    def __init__(self, test_name, test_type, test_params=default_param):
        super(FIRTest, self).__init__(test_name, test_type, test_params)
        self.params = test_params

    def run(self):
        print(f"{self.test_name} started.")
        self.run_file_test()

    def run_file_test(self):
        ffi = FFI()
        ffi.cdef('''
            float* fir(float* input, float* kernel, float* output, long inputSize, long kernelSize, long outputSize);
        ''')
        lib_path = lp.format_lib_path(self.params['lib'])
        C = ffi.dlopen(lib_path)

        sample_rate, sp_nasa = sp.io.wavfile.read(self.params['file'])

        firfilt = sp.signal.firwin(
            10, 0.1, window=self.params['fconf'], pass_zero='lowpass', scale=False).astype(np.float32)

        sp_nasa = af.pcm2float(sp_nasa, dtype='float32')

        grpdelay = sp.signal.group_delay((firfilt, 1.0))
        delay = round(np.mean(grpdelay[1]), 2)
        # buddy fir filtering
        input = ffi.cast("float *", ffi.from_buffer(sp_nasa))
        kernel = ffi.cast("float *", firfilt.ctypes.data)
        output = ffi.new("float[]", sp_nasa.size)
        print(f"input size: {sp_nasa.size}")
        out = C.fir(input, kernel, output,
                    sp_nasa.size, firfilt.size, sp_nasa.size)
        print("fir finished")
        out = ffi.unpack(out, sp_nasa.size)

        # scipy fir filtering
        out_sp = sp.signal.lfilter(firfilt, 1, sp_nasa)

        # numpy fir filtering
        out_np = np.convolve(sp_nasa, firfilt, mode='full')
        
        pwd = os.getcwd()
        print(f"Writing files to {pwd}/fir_scipy_buddy.txt")

        file = open("fir_scipy_buddy.txt", "w")
        file.write(f"buddy\tscipy\n")

        diff = math.floor(2*delay)
        for i in range(sp_nasa.size-diff):
            file.write(f"{out[i]}\t{out_sp[i+diff]}\n")
            if (abs(out[i] - out_sp[i+diff]) > 10e-6):
                print(f"scipy and buddy are different at {i}")
                print("check failed.")
                sys.exit(1)
        print(f"{self.test_name} check successful.")
