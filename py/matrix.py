import numpy as np

def translate(pos):
    tx, ty, tz = pos
    return np.array([[1, 0, 0, 0],
                    [0, 1, 0, 0],
                    [0, 0, 1, 0],
                    [tx, ty, tz, 1]])

def rotate_x(angle):
    return np.array([
        [1, 0, 0, 0],
        [0, np.cos(angle), np.sin(angle), 0],
        [0, -np.sin(angle), np.cos(angle), 0],
        [0, 0, 0, 1]])

def rotate_y(angle):
    return np.array([[np.cos(angle), 0, -np.sin(angle), 0],
                     [0, 1, 0, 0],
                     [np.sin(angle), 0, np.cos(angle), 0],
                     [0, 0, 0, 1]])

def scale(zoom):
    return np.array([[zoom, 0, 0, 0],
                     [0, zoom, 0, 0],
                     [0, 0, zoom, 0],
                     [0, 0, 0, 1]])
