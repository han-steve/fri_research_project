import numpy as np
import os
from torchvision import datasets, transforms
import torch
from PIL import Image


class PoolTable(object):
    """Data Handler that read our generated dataset."""

    def __init__(self, data_root="/100_data_small", seq_len=10, image_size=64):
        self.seq_len = seq_len
        self.image_size = image_size
        self.channels = 1
        self.path = os.getcwd() + data_root
        self.files = list(
            map(lambda file: os.path.join(self.path, file), os.listdir(self.path)))

    def __len__(self):
        return len(self.files)

    def __getitem__new(self, index):
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

    def __getitem__(self, index):
        # x = np.empty((self.seq_len,
        #               self.image_size,
        #               self.image_size,
        #               self.channels),
        #              dtype=np.float32)
        pos = len(self.path) + 1
        label = int(self.files[index][pos:pos+1])
   
        # label = torch.tensor([1, 0]) if label == 0 else torch.tensor([0, 1])
        x_arr = []
        with open(self.files[index], 'rb') as f:
            for frame in range(self.seq_len):
                img = Image.new('L', (self.image_size, self.image_size))
                pixels = img.load()
                x = 0
                y = self.image_size - 1
                rgb = ()
                while (byte := f.read(1)):
                    if len(rgb) == 3:
                        pixels[x, y] = sum(rgb) // len(rgb)
                        rgb = ()
                        x += 1
                        if x == self.image_size:
                            y -= 1
                            x = 0
                        if y == -1:
                            break
                    rgb += (int.from_bytes(byte, "little"),)

                # height x width x channel
                arr = np.array(img).astype(np.float32)
                arr /= 255

                x_arr.append(arr.flatten())
        np_x = np.array(x_arr, dtype=np.float32).reshape((self.seq_len,
                                                      self.image_size,
                                                      self.image_size,
                                                      self.channels))
        return (np_x , label)
