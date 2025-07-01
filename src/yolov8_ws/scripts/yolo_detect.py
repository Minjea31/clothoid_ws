#!/usr/bin/env python3
import rospy
import numpy as np
import cv2
from sensor_msgs.msg import CompressedImage
from std_msgs.msg import Header
from detect_msgs.msg import Yolo_Objects, Objects
from ultralytics import YOLO

class YoloDetectNode:
    def __init__(self):
        rospy.init_node("yolo_detect_node")

        # params
        source_topic  = rospy.get_param("~source", "/camera/image_raw/compressed")
        yaml_cfg      = rospy.get_param("~yaml_cfg", "./best.yaml")
        pt_weights    = rospy.get_param("~pt_weights", "./best.pt")
        self.conf_thres = rospy.get_param("~confidence", 0.7)

        # publisher
        self.pub = rospy.Publisher("yolov8_pub", Yolo_Objects, queue_size=1)

        # 모델 로드
        self.model = YOLO(yaml_cfg, task='detect').load(pt_weights)
        rospy.loginfo(f"[yolo_detect_node] Model loaded: {yaml_cfg}, {pt_weights}")

        # 구독
        rospy.Subscriber(source_topic, CompressedImage, self.callback,
                         queue_size=1, buff_size=2**24)
        rospy.loginfo(f"[yolo_detect_node] Subscribed to {source_topic}")

    def callback(self, msg: CompressedImage):
        # 1) 압축 해제
        np_arr = np.frombuffer(msg.data, np.uint8)
        frame  = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
        h0, w0 = frame.shape[:2]

        # 2) 추론
        results = self.model(frame, imgsz=(h0, w0), conf=self.conf_thres)[0]

        # 3) Yolo_Objects 메시지 구성
        out = Yolo_Objects()
        out.header = Header(stamp=msg.header.stamp,
                            frame_id=msg.header.frame_id or "camera_link")

        # Objects 배열 채우기
        for idx, box in enumerate(results.boxes):
            cls_id = int(box.cls.cpu().item())
            x1, y1, x2, y2 = box.xyxy[0].cpu().tolist()
            obj = Objects()
            obj.Class = cls_id
            obj.id    = idx
            obj.x1    = int(x1)
            obj.y1    = int(y1)
            obj.x2    = int(x2)
            obj.y2    = int(y2)
            out.yolo_objects.append(obj)

        # 4) 퍼블리시
        self.pub.publish(out)

    def spin(self):
        rospy.spin()

if __name__ == "__main__":
    node = YoloDetectNode()
    node.spin()

