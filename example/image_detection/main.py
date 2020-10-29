import cv2
import torch
import torchvision
import numpy as np
from lamppost import reader
import random

video_path = reader.VideoStream("/thermal/trafi_one_195/stream/1/lab/test1").get_stream_path()

device = "cuda" if torch.cuda.is_available() else "cpu"
model = torchvision.models.detection.fasterrcnn_resnet50_fpn(
    pretrained=True).eval().to(device)

# 定义 Pytorch 官方给的类别名称，有些是 'N/A' 是已经去掉的类别
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
    pred = model([img.to(device)])
    pred_class = [COCO_INSTANCE_CATEGORY_NAMES[i] for i in list(pred[0]['labels'].cpu().numpy())]
    pred_boxes = [[(i[0], i[1]), (i[2], i[3])] for i in list(pred[0]['boxes'].detach().cpu().numpy())] 
    pred_score = list(pred[0]['scores'].cpu().detach().numpy())
    pred_t = [pred_score.index(x) for x in pred_score if x > threshold][-1]
    pred_boxes = pred_boxes[:pred_t+1]  
    pred_class = pred_class[:pred_t+1]
    return pred_boxes, pred_class


def object_detection_api(img, threshold=0.5, rect_th=3, text_size=3, text_th=3):
    boxes, pred_cls = get_prediction(img, threshold)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    result_dict = {}  #  用来保存每个类别的名称及数量
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

i = 0
vid = cv2.VideoCapture(video_path)
vid.open()
while True:
    valid, img = vid.read()
    if valid:
        pimg = get_prediction(img, 0.5)
        cv2.imwrite("%d.jpg" % i, pimg)
        i += 1
