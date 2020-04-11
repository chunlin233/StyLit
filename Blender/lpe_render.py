'''
This script renders the Light Path Expressions   \

USAGE : blender --background <filename>.blend --factory-startup --python lpe_render.py -- 
--type=full --model=Sphere --output_dir=./ --resolution=1200x912
'''

import bpy
from bpy import context, data, ops

import sys
import os

import argparse 
import math


class Blender():
    def __init__(self):
        self.model = None
    def import_model(self, path):
        old_obj = set(context.scene.objects)
        bpy.ops.import_scene.obj(filepath=path, filter_glob="*.obj")
        self.model = (set(context.scene.objects) - old_obj).pop()
    def set_material(self):
        pass
    def render_modality(self, lpe_type, output_pth, width, height):
        context.scene.render.resolution_x = width
        context.scene.render.resolution_y = height
        context.scene.render.resolution_percentage = 100
        context.scene.render.filepath = os.path.join(output_pth, lpe_type + '.png')
        bpy.ops.render.render(write_still=True)

def get_objs_in_directory(path):
    objs = []
    for f in os.listdir(path):
        f_split  = os.path.splitext(f)
        f_name = f_split[0]
        ext = f_split[1]
        if ext.lower() not in ['.obj']:
            continue
        objs.append(f)
    return objs

def execute_blender(lpe_type, model_pth, output_pth, resolution):
    # DATA = get_objs_in_directory(path)
    blender_instance = Blender()
    if model_pth != "Sphere":
        blender_instance.import_model(model_pth)
    filename = model_pth.split('.')[0].split('/')[-1]
    context.view_layer.objects.active = bpy.data.objects[filename]
    width = int(resolution.split('x')[0])
    height = int(resolution.split('x')[1])
    blender_instance.render_modality(lpe_type, output_pth, width, height)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.data.objects[filename].select_set(True)
    bpy.ops.object.delete()


def main():

    argv = sys.argv

    if "--" not in argv:
        argv = []  # as if no args are passed
    else:
        argv = argv[argv.index("--") + 1:]  # get all args after "--"

    # When --help or no args are given, print this help
    usage_text = (
        "Run blender in background mode with this script:"
        "  blender --background --python " + __file__ + " -- [options]"
    )

    parser = argparse.ArgumentParser(description=usage_text)

    parser.add_argument(
        "-t", "--type", dest="type", type=str, required=True,
        help="What LPE do you want to render? Options {full, dirdiff, dirspec, diff2b, diffinter}",
    )

    parser.add_argument(
        "-m", "--model", dest="model", type=str, required=True,
        help="Input 3D model path: Options {Sphere, <path to .obj file>}",
    )
    parser.add_argument(
        "-o", "--output_dir", dest="output_dir", type=str, required=True,
        help="Output rendered directory",
    )
    parser.add_argument(
        "-r", "--resolution", dest="resolution", type=str, required=True,
        help="Output resolution: Options {1200x912, ...}",
    )

    args = parser.parse_args(argv)

    if not argv:
        parser.print_help()
        return

    if (not args.type or 
        not args.model or 
        not args.output_dir or 
        not args.resolution):
        print("Error: argument not given, aborting.")
        parser.print_help()
        return

    if args.model != "Sphere" and os.path.isfile(args.model):
        print("Invalid .obj path: Path does not exist.")
        return 


    execute_blender(args.type, args.model, args.output_dir, args.resolution)

if __name__ == "__main__":
    main()
