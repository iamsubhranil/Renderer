import pygame as pg
from camera import *
from projection import *
from object3d import *
import sys

class SoftwareRenderer:
    def __init__(self):
        pg.init()
        self.RES = self.WIDTH, self.HEIGHT = 1024, 720
        self.H_WIDTH, self.H_HEIGHT = self.WIDTH // 2, self.HEIGHT // 2
        self.FPS = 144
        self.screen = pg.display.set_mode(self.RES)
        self.clock = pg.time.Clock()
        self.create_objects()

    def create_objects(self):
        self.camera = Camera(self, [-153.44, 740.24, -2276.96])
        self.projection = Projection(self)
        if len(sys.argv) < 2:
            self.object = Object3D(self)
            self.object.translate([0.2, 0.4, 0.2])
        else:
            self.object = self.load_object(sys.argv[1])

    def load_object(self, filename):
        vertices, faces = [], []
        with open(filename) as f:
            for line in f:
                if line.startswith('v '):
                    vertices.append([float(i) for i in line.split()[1:]] + [1])
                elif line.startswith('f'):
                    faces_ = line.split()[1:]
                    faces.append([int(face_.split("/")[0]) - 1 for face_ in faces_])
        return Object3D(self, vertices, faces)

    def draw(self):
        self.screen.fill(pg.Color('black'))
        self.object.draw()

    def run(self):
        while True:
            self.draw()
            self.camera.control()
            [exit() for i in pg.event.get() if i.type == pg.QUIT]
            print("\rFPS:", str(self.clock.get_fps()), end="\r")
            pg.display.flip()
            self.clock.tick(self.FPS)


if __name__ == "__main__":
    app = SoftwareRenderer()
    app.run()
