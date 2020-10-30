import cv2
import torch
import torchvision
import numpy as np
from lamppost import reader
import random
import queue
import threading
import time
import sys

video_path = reader.VideoStream(
    "/thermal/trafi_one_195/stream/1/lab/test1").get_stream_path()

device = "cuda" if torch.cuda.is_available() else "cpu"
model = torchvision.models.detection.fasterrcnn_resnet50_fpn(
    pretrained=True).eval().to(device)

COCO_INSTANCE_CATEGORY_NAMES = [
    '__background__', 'person', 'bicycle', 'car', 'motorcycle', 'airplane', 'bus',
    'train', 'truck', 'boat', 'traffic light', 'fire hydrant', 'N/A', 'stop sign',
    'parking meter', 'bench', 'bird', 'cat', 'dog', 'horse', 'sheep', 'cow',
    'elephant', 'bear', 'zebra', 'giraffe', 'N/A', 'backpack', 'umbrella', 'N/A', 'N/A',
    'handbag', 'tie', 'suitcase', 'frisbee', 'skis', 'snowboard', 'sports ball',
    'kite', 'baseball bat', 'baseball glove', 'skateboard', 'surfboard', 'tennis racket',
    'bottle', 'N/A', 'wine glass', 'cup', 'fork', 'knife', 'spoon', 'bowl',
    'banana', 'apple', 'sandwich', 'orange', 'broccoli', 'carrot', 'hot dog', 'pizza',
    'donut', 'cake', 'chair', 'couch', 'potted plant', 'bed', 'N/A', 'dining table',
    'N/A', 'N/A', 'toilet', 'N/A', 'tv', 'laptop', 'mouse', 'remote', 'keyboard', 'cell phone',
    'microwave', 'oven', 'toaster', 'sink', 'refrigerator', 'N/A', 'book',
    'clock', 'vase', 'scissors', 'teddy bear', 'hair drier', 'toothbrush'
]


def get_prediction(img, threshold):
    img_tensor = torch.from_numpy(img/255.).permute(2, 0, 1).float().to(device)
    pred = model([img_tensor])
    pred_class = [COCO_INSTANCE_CATEGORY_NAMES[i]
                  for i in list(pred[0]['labels'].cpu().numpy())]
    pred_boxes = [[(i[0], i[1]), (i[2], i[3])]
                  for i in list(pred[0]['boxes'].detach().cpu().numpy())]
    pred_score = list(pred[0]['scores'].cpu().detach().numpy())
    pred_ts = [pred_score.index(x) for x in pred_score if x > threshold]
    if len(pred_ts) == 0:
        return [], []
    pred_t = pred_ts[-1]
    pred_boxes = pred_boxes[:pred_t+1]
    pred_class = pred_class[:pred_t+1]
    return pred_boxes, pred_class


def object_detection_api(img, threshold=0.5, rect_th=3, text_size=3, text_th=3):
    boxes, pred_cls = get_prediction(img, threshold)
    # img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    result_dict = {}
    for i in range(len(boxes)):
        color = tuple(random.randint(0, 255) for i in range(3))
        cv2.rectangle(img,
                      boxes[i][0],
                      boxes[i][1],
                      color=color,
                      thickness=rect_th)  # Draw Rectangle with the coordinates
        cv2.putText(img,
                    pred_cls[i],
                    boxes[i][0],
                    cv2.FONT_HERSHEY_SIMPLEX,
                    text_size,
                    color,
                    thickness=text_th)  # Write the prediction class
        if pred_cls[i] not in result_dict:
            result_dict[pred_cls[i]] = 1
        else:
            result_dict[pred_cls[i]] += 1
        print(result_dict)
    return img


class VideoCapture:
    def __init__(self, name):
        self.cap = cv2.VideoCapture(name)
        self.q = queue.Queue()
        self.t = threading.Thread(target=self._reader)
        self.t.daemon = True
        self.t.start()
    
    def stop(self):
        self.t.do_run = False
        self.t.join()

    # read frames as soon as they are available, keeping only most recent one
    def _reader(self):
        t = threading.currentThread()
        while getattr(t, "do_run", True):
            ret, frame = self.cap.read()
            if not ret:
                break
            if not self.q.empty():
                try:
                    self.q.get_nowait()   # discard previous (unprocessed) frame
                except queue.Empty:
                    pass
            self.q.put(frame)

    def read(self):
        return self.q.get()


i = 0
vid = VideoCapture(video_path)
while i < 10:
    img = vid.read()
    if img is not None:
        pimg = object_detection_api(img, 0.5)
        cv2.imwrite("%d.jpg" % i, pimg)
        i += 1
    else:
        print("Cannot Read Video")
vid.stop()

import glob
import ftplib
# ftp = ftplib.FTP(host='137.189.97.26', source_address=('192.168.8.100', 21101))

ftp = ftplib.FTP()
ftp.connect(host='137.189.97.26')
ftp.login('nvidia', 'nvidia')
ftp.cwd('kit')
for path in glob.glob(r'*.jpg'):
    with open(path, 'rb') as f:
        ftp.storbinary('STOR %s' % path, f)

ftp.quit()

sys.exit(0)
