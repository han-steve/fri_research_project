import numpy as np
import os
from torchvision import datasets, transforms
from PIL import Image


class PoolTable(object):
    """Data Handler that read our generated dataset."""

    def __init__(self, data_root="/100_data", seq_len=30, image_size=256):
        self.seq_len = seq_len
        self.image_size = image_size
        self.channels = 1
        path = os.getcwd() + data_root
        self.files = list(
            map(lambda file: os.path.join(path, file), os.listdir(path)))

    def __len__(self):
        return len(self.files)

    def __getitem__(self, index):
        # x = np.empty((self.seq_len,
        #               self.image_size,
        #               self.image_size,
        #               self.channels),
        #              dtype=np.float32)
        x_arr = []
        with open(self.files[index], 'rb') as f:
            i = 0
            rgb = ()
            while (byte := f.read(1)):
                if (i > self.seq_len * self.image_size * self.image_size * 3):
                    break
                if len(rgb) == 3:
                    x_arr.append(sum(rgb) / len(rgb) / 255)
                    rgb = ()
                rgb += (int.from_bytes(byte, "little"),)
                i += 1

        np_x = np.array(x_arr, dtype=np.float32).reshape((self.seq_len,
                                                      self.image_size,
                                                      self.image_size,
                                                      self.channels))
        print (np_x.shape)
        return np_x

    def __getitem__old__(self, index):
        # x = np.empty((self.seq_len,
        #               self.image_size,
        #               self.image_size,
        #               self.channels),
        #              dtype=np.float32)
        x_arr = []
        with open(self.files[index], 'rb') as f:
            for frame in range(self.seq_len):
                img = Image.new('L', (800, 800))
                pixels = img.load()
                x = 0
                y = 799
                rgb = ()
                while (byte := f.read(1)):
                    if len(rgb) == 3:
                        pixels[x, y] = sum(rgb) // len(rgb)
                        rgb = ()
                        x += 1
                        if x == 800:
                            y -= 1
                            x = 0
                        if y == -1:
                            break
                    rgb += (int.from_bytes(byte, "little"),)

                img = img.resize((self.image_size, self.image_size), Image.ANTIALIAS)
                # height x width x channel
                arr = np.array(img).astype(np.float32)
                arr /= 255

                x_arr.append(arr.flatten())
        np_x = np.array(x_arr, dtype=np.float32).reshape((self.seq_len,
                                                      self.image_size,
                                                      self.image_size,
                                                      self.channels))
        return np_x
