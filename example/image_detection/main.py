import cv2
import torch
import torchvision
from lamppost import reader

# https://juejin.im/post/6865120736982335501
model = torchvision.models.detection.fasterrcnn_resnet50_fpn(
    pretrained=True)

# images, boxes = torch.rand(4, 3, 600, 1200), torch.rand(4, 11, 4)
# labels = torch.randint(1, 91, (4, 11))
# images = list(image for image in images)
# targets = []
# for i in range(len(images)):
#     d = {}
#     d['boxes'] = boxes[i]
#     d['labels'] = labels[i]
#     targets.append(d)
# output = model(images, targets)

# For inference
model.eval()

video_path = reader.VideoStream("/thermal/trafi_one_195/stream/1/lab/test1")
vid = cv2.VideoCapture(video_path)
vid.open()
while True:
    img = vid.read()
    if img:
        x = [torch.rand(3, 500, 400)]
        predictions = model(x)
        print(predictions)
