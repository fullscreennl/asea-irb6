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
from periphery import GPIO

ser = None
nullframe_appearance = 0
averages = []
# gpio_out = GPIO(73, "out") # face detected no/yes
# gpio_lr  = GPIO(138, "out") # moveleft /right
# gpio_ud  = GPIO(140, "out") # dont move right nor left
# gpio_out.write(False)
# gpio_lr.write(False)
# gpio_ud.write(False)

# state table
"""
0 NO FACE
1 NOMOVE NOMOVE
2 NOMOVE UP
3 NOMOVE DOWN
4 LEFT NOMOVE
5 LEFT UP
6 LEFT DOWN
7 RIGHT NOMOVE
8 RIGHT UP
9 RIGHT DOWN
"""
class Constants:
    NO_FACE = 0
    X_X = 1
    X_U = 2
    X_D = 3
    L_X = 4
    L_U = 5
    L_D = 6
    R_X = 7
    R_U = 8
    R_D = 9

direction_names = ['NO_FACE','X_X','X_U','X_D','L_X','L_U','L_D','R_X','R_U','R_D']

centered = False 

gpio_line3 = GPIO(73, "out")
gpio_line1 = GPIO(138, "out")
gpio_line2 = GPIO(140, "out")
gpio_line0 = GPIO(6, "out") 
gpio_line0.write(False)
gpio_line1.write(False)
gpio_line2.write(False)
gpio_line3.write(False)
current_state = 0  # NO FACE
hysterysX = 0
hysterysY = 0
centerpoint = 0,0
gpios = [gpio_line0,gpio_line1,gpio_line2,gpio_line3]
        
def dec2bin(n):
    return bin(n).replace("0b", "").zfill(4)  

def writeIO(t):
    global gpios
    binString = dec2bin(t)
    for count, a in enumerate(binString):
        gpios[count].write(bool(int(a)))

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
    dwg.add(dwg.rect(insert=(1920/2 -120,0), size=(240,1280), fill='none', stroke='grey', stroke_width='2'))
    src_w, src_h = src_size
    inf_w, inf_h = inference_size
    box_x, box_y, box_w, box_h = inference_box
    scale_x, scale_y = src_w / box_w, src_h / box_h
    largest_width = 0
    largest_height = 0
    largest_x = 0
    largest_y = 0
    center_x = 0
    center_y = 0
    global nullframe_appearance, averages, hysterysX, hysterysY, centeredX, centerpoint, current_state
    # for y, line in enumerate(text_lines, start=1):
    #     shadow_text(dwg, 10, y*20, line)
    if len(objs) == 0:
        nullframe_appearance += 1
        # gpio_out.write(False)
        current_state = Constants.NO_FACE # NO FACE
        writeIO(current_state)
        # print('face not detected')
    else:
        nullframe_appearance = 0
        # gpio_out.write(True)
        current_state = Constants.X_X # NO MOVE NO MOVE
        # print('face detected')
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
        # largest face storing xvalue for averaging
        if w > largest_width:
            largest_width = w
            largest_height = h
            largest_x = x
            largest_y = y
            center_x = largest_x + int(float(largest_width)/2.0)
            center_y = largest_y + int(float(largest_height)/2.0)
            averages.append((center_x, center_y))
            averages = averages[-20:]
            centerpoint = calc_average(averages)
        dwg.add(dwg.circle( center=centerpoint, r=5, fill='red')) 
        # print(output_frame.encode('utf-8'))

    #if centerpoint[0] > (1920/2):
        #gpio_lr.write(True)
    #else:
        #gpio_lr.write(False)
    dwg.add(dwg.circle( center=(10,10), r=5, fill='green')) 
    deltaX = abs((1920/2) - centerpoint[0])
    deltaY = abs((1080/2) - centerpoint[1])
    if current_state > 0 and (deltaX > (20 + hysterysX) or deltaY > (20 + hysterysY)):
        # face off center 
        if deltaX > (20 + hysterysX):
            centeredX = False
            hysterysX = 0
            if centerpoint[0] > (1920/2):
                # move left
                if deltaY <= (20 + hysterysY):
                    # dont move up or down
                    hysterysY = 100
                    current_state = Constants.L_X
                elif centerpoint[1] > 1080/2:
                    # move down to follow
                    hysterysY = 0
                    current_state = Constants.L_D
                else:
                   # move up
                   hysterysY = 0
                   current_state = Constants.L_U
            else:
                # move right
                # gpio_ud.write(False) # start moving
                if deltaY <= (20 + hysterysY):
                    # dont move up or down
                    hysterysY = 100
                    current_state = Constants.R_X
                elif centerpoint[1] > 1080/2:
                    # move down
                    current_state = Constants.R_D
                    hysterysY = 0
                else:
                   # move up
                   hysterysY = 0
                   current_state = Constants.R_U
        elif deltaY > (20 + hysterysY):
            hysterysX = 100
            hysterysY = 0
            # up or down and no left or right movement 
            if centerpoint[1] > 1080/2:
                # move down
                current_state = Constants.X_D
            else:
               # move up
               current_state = Constants.X_U
    else:
        # face in center, enlarge deadzone to prevent jitter
        hysterysY = 100
        hysterysX = 100
        centeredX = True
        #gpio_ud.write(True) # no move
    dwg.add(dwg.rect(insert=(largest_x,largest_y), size=(largest_width,int(largest_height)),fill='none',stroke='white',stroke_width='4'))
    if nullframe_appearance > 10 or nullframe_appearance == 0:
        if nullframe_appearance > 10:
            averages_x = []
        nullframe_appearance = 0
    # print('sending state', direction_names[current_state])    
    writeIO(current_state)    
    return dwg.tostring()

def calc_average(data):
    if len(data) == 0:
        return (0,0)
    total = 0
    x = 0
    y = 0
    for _x,_y in data:
        x += _x
        y += _y
    avg_x = round(x/len(data))
    avg_y = round(y/len(data))
    # print (avg)
    return avg_x,avg_y


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
    global gpio_out, current_state
    w, h, _ = common.input_image_size(interpreter)
    inference_size = (w, h)
    # Average fps over last 30 frames.
    fps_counter  = common.avg_fps_counter(30)
    counter = 0
    def user_callback(input_tensor, src_size, inference_box):
      nonlocal fps_counter
      nonlocal counter
      global gpio_out, current_state
      counter += 1
      if counter == 5000:
          # gpio_out.write(False)
          current_state = 0
          writeIO(current_state)    
          os._exit(1)
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
