# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A demo which runs object detection on camera frames using GStreamer.

TEST_DATA=../all_models

Run face detection model:
python3 detect.py \
  --model ${TEST_DATA}/mobilenet_ssd_v2_face_quant_postprocess_edgetpu.tflite

sudo -E python3 detect.py --model all_models/mobilenet_ssd_v2_face_quant_postprocess_edgetpu.tflite

Run coco model:
python3 detect.py \
  --model ${TEST_DATA}/mobilenet_ssd_v2_coco_quant_postprocess_edgetpu.tflite \
  --labels ${TEST_DATA}/coco_labels.txt
"""
import argparse
import collections
import common
import gstreamer
import numpy as np
import os
import re
import svgwrite
import time
import serial

ser = None
nullframe_appearance = 0

def init():
    global ser
    ser = serial.Serial(
        port='/dev/ttymxc2', #Replace ttyS0 with ttyAMA0 for Pi1,Pi2,Pi0
        baudrate = 9600, # match with couterpart receiving code
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
    )
    counter=0

#while 1:
#        ser.write("Write counter: %d \n"%(counter))
#        time.sleep(1)
#        counter += 1

Object = collections.namedtuple('Object', ['id', 'score', 'bbox'])

def load_labels(path):
    p = re.compile(r'\s*(\d+)(.+)')
    with open(path, 'r', encoding='utf-8') as f:
       lines = (p.match(line).groups() for line in f.readlines())
       return {int(num): text.strip() for num, text in lines}

def shadow_text(dwg, x, y, text, font_size=20):
    dwg.add(dwg.text(text, insert=(x+1, y+1), fill='black', font_size=font_size))
    dwg.add(dwg.text(text, insert=(x, y), fill='white', font_size=font_size))

def generate_svg(src_size, inference_size, inference_box, objs, labels, text_lines):
    dwg = svgwrite.Drawing('', size=src_size)
    src_w, src_h = src_size
    inf_w, inf_h = inference_size
    box_x, box_y, box_w, box_h = inference_box
    scale_x, scale_y = src_w / box_w, src_h / box_h
    largest_width = 0
    largest_height = 0
    largest_x = 0
    largest_y = 0
    global nullframe_appearance
    # for y, line in enumerate(text_lines, start=1):
    #     shadow_text(dwg, 10, y*20, line)
    if len(objs) == 0:
        nullframe_appearance += 1
    else:
        nullframe_appearance = 0
    for obj in objs:
        x0, y0, x1, y1 = list(obj.bbox)
        # Relative coordinates.
        x, y, w, h = x0, y0, x1 - x0, y1 - y0
        # Absolute coordinates, input tensor space.
        x, y, w, h = int(x * inf_w), int(y * inf_h), int(w * inf_w), int(h * inf_h)
        # Subtract boxing offset.
        x, y = x - box_x, y - box_y
        # Scale to source coordinate space.
        x, y, w, h = x * scale_x, y * scale_y, w * scale_x, h * scale_y
        percent = int(100 * obj.score)
        label = '{}% {}'.format(percent, labels.get(obj.id, obj.id))
        # shadow_text(dwg, x, y - 5, label)
        # dwg.add(dwg.rect(insert=(x,y), size=(w, h),
        #                 fill='none', stroke='grey', stroke_width='2'))
        #dwg.add(dwg.rect(insert=(x,y+int(h/3)), size=(w,int(h/7)),fill='black',stroke='black',stroke_width='2'))
        if w > largest_width:
            largest_width = w
            largest_height = h
            largest_x = x
            largest_y = y
        # print(output_frame.encode('utf-8'))

    output_frame = "#%04d,%04d,%04d,%04d*"%(largest_x,largest_y,largest_width,largest_height)
    # print(output_frame)
    dwg.add(dwg.rect(insert=(largest_x,largest_y), size=(largest_width,int(largest_height)),fill='none',stroke='white',stroke_width='4'))
    if nullframe_appearance > 10 or nullframe_appearance == 0:
        nullframe_appearance = 0
        print(output_frame)
        ser.write(output_frame.encode('utf-8'))
    return dwg.tostring()

class BBox(collections.namedtuple('BBox', ['xmin', 'ymin', 'xmax', 'ymax'])):
    """Bounding box.
    Represents a rectangle which sides are either vertical or horizontal, parallel
    to the x or y axis.
    """
    __slots__ = ()

def get_output(interpreter, score_threshold, top_k, image_scale=1.0):
    """Returns list of detected objects."""
    boxes = common.output_tensor(interpreter, 0)
    category_ids = common.output_tensor(interpreter, 1)
    scores = common.output_tensor(interpreter, 2)

    def make(i):
        ymin, xmin, ymax, xmax = boxes[i]
        return Object(
            id=int(category_ids[i]),
            score=scores[i],
            bbox=BBox(xmin=np.maximum(0.0, xmin),
                      ymin=np.maximum(0.0, ymin),
                      xmax=np.minimum(1.0, xmax),
                      ymax=np.minimum(1.0, ymax)))
    return [make(i) for i in range(top_k) if scores[i] >= score_threshold]

def main():
    default_model_dir = 'all_models'
    default_model = 'mobilenet_ssd_v2_coco_quant_postprocess_edgetpu.tflite'
    default_labels = 'coco_labels.txt'
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', help='.tflite model path',
                        default=os.path.join(default_model_dir,default_model))
    parser.add_argument('--labels', help='label file path',
                        default=os.path.join(default_model_dir, default_labels))
    parser.add_argument('--top_k', type=int, default=3,
                        help='number of categories with highest score to display')
    parser.add_argument('--threshold', type=float, default=0.1,
                        help='classifier score threshold')
    args = parser.parse_args()

    print('Loading {} with {} labels.'.format(args.model, args.labels))
    interpreter = common.make_interpreter(args.model)
    interpreter.allocate_tensors()
    labels = load_labels(args.labels)

    w, h, _ = common.input_image_size(interpreter)
    inference_size = (w, h)
    # Average fps over last 30 frames.
    fps_counter  = common.avg_fps_counter(30)

    def user_callback(input_tensor, src_size, inference_box):
      nonlocal fps_counter
      start_time = time.monotonic()
      common.set_input(interpreter, input_tensor)
      interpreter.invoke()
      # For larger input image sizes, use the edgetpu.classification.engine for better performance
      objs = get_output(interpreter, args.threshold, args.top_k)
      end_time = time.monotonic()
      text_lines = [
          'Inference: {:.2f} ms'.format((end_time - start_time) * 1000),
          'FPS: {} fps'.format(round(next(fps_counter))),
      ]
      #print(' '.join(text_lines))
      return generate_svg(src_size, inference_size, inference_box, objs, labels, text_lines)
    print(inference_size, gstreamer)
    result = gstreamer.run_pipeline(user_callback, appsink_size=inference_size)

if __name__ == '__main__':
    init()
    main()
