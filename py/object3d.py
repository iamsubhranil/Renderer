import pygame as pg
from pygame import gfxdraw
from matrix import *
from numba import njit
import random

@njit(fastmath=True)
def any_func(arr, a, b):
    return np.any((arr == a) | (arr == b))

def random_color():
    levels = range(32,256,5)
    return tuple(random.choice(levels) for _ in range(3))

class Object3D:
    def __init__(self, renderer, vertices = None, faces = None):
        self.renderer = renderer
        if vertices == None:
            self.vertices = np.array([[0, 0, 0, 1], [0, 1, 0, 1], [1, 1, 0, 1], [1, 0, 0, 1],
                           [0, 0, 1, 1], [0, 1, 1, 1], [1, 1, 1, 1], [1, 0, 1, 1]])
        else:
            # for i, vertex in enumerate(vertices):
            #    print("vertex", str(i) + ":", *vertex[:-1])
            self.vertices = np.array([np.array(v) for v in vertices])
        if faces == None:
            self.faces = np.array([(0, 1, 2, 3), (4, 5, 6, 7), (0, 4, 5, 1), (2, 3, 7, 6),
                                   (1, 2, 6, 5), (0, 3, 7, 4)])
        else:
            # for i, face in enumerate(faces):
            #    print("face", str(i) + ":", *face)
            self.faces = np.array([np.array(face) for face in faces])

        self.font = pg.font.SysFont('OpenSans', 30, bold=True)
        self.color_faces = [(random_color(), face) for face in self.faces]
        self.movement_flag, self.draw_vertices = True, True
        self.label = ''

    def draw(self):
        self.screen_projection()
        # self.movement()

    def movement(self):
        if self.movement_flag:
            self.rotate_y(pg.time.get_ticks() % 0.005)

    def screen_projection(self):
        print(self.vertices, "\n")
        print(self.renderer.camera.camera_matrix(), "\n")
        vertices = self.vertices @ self.renderer.camera.camera_matrix()
        print(vertices, "\n")
        print(self.renderer.projection.projection_matrix, "\n")
        vertices = vertices @ self.renderer.projection.projection_matrix
        print(vertices, "\n")
        # print(vertices)
        vertices /= vertices[:, -1].reshape(-1, 1)
        vertices[(vertices > 1) | (vertices < -1)] = 0
        print(vertices, "\n")
        print(self.renderer.projection.to_screen_matrix, "\n")
        vertices = vertices @ self.renderer.projection.to_screen_matrix
        print(vertices, "\n")
        # input()
        vertices = vertices[:, :2]

        for index, color_face in enumerate(self.color_faces):
            color, face = color_face
            polygon = vertices[face]
            if not any_func(polygon, self.renderer.H_WIDTH, self.renderer.H_HEIGHT):
                pg.draw.polygon(self.renderer.screen, color, polygon, 1)
            #    if self.label:
            #        text = self.font.render(self.label[index], True, pg.Color('white'))
            #        self.renderer.screen.blit(text, polygon[-1])

        if self.draw_vertices:
            for vertex in vertices:
                if not any_func(vertex, self.renderer.H_WIDTH, self.renderer.H_HEIGHT):
                    pg.draw.circle(self.renderer.screen, pg.Color('white'), vertex, 2)

    def translate(self, pos):
        self.vertices = self.vertices @ translate(pos)

    def scale(self, zoom):
        self.vertices = self.vertices @ scale(zoom)

    def rotate_x(self, angle):
        self.vertices = self.vertices @ rotate_x(angle)

    def rotate_y(self, angle):
        self.vertices = self.vertices @ rotate_y(angle)

    def rotate_z(self, angle):
        self.vertices = self.vertices @ rotate_z(angle)


class Axes(Object3D):
    def __init__(self, renderer):
        super().__init__(renderer)
        self.vertices = np.array([(0, 0, 0, 1), (1, 0, 0, 1), (0, 1, 0, 1), (0, 0, 1, 1)])
        self.faces = np.array([(0, 1), (0, 2), (0, 3)])
        self.colors = [pg.Color('red'), pg.Color('green'), pg.Color('blue')]
        self.color_faces = [(color, face) for color, face in zip(self.colors, self.faces)]
        self.draw_vertices = False
        self.label = 'XYZ'
