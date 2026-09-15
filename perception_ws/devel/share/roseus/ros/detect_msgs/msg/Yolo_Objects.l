;; Auto-generated. Do not edit!


(when (boundp 'detect_msgs::Yolo_Objects)
  (if (not (find-package "DETECT_MSGS"))
    (make-package "DETECT_MSGS"))
  (shadow 'Yolo_Objects (find-package "DETECT_MSGS")))
(unless (find-package "DETECT_MSGS::YOLO_OBJECTS")
  (make-package "DETECT_MSGS::YOLO_OBJECTS"))

(in-package "ROS")
;;//! \htmlinclude Yolo_Objects.msg.html
(if (not (find-package "STD_MSGS"))
  (ros::roseus-add-msgs "std_msgs"))


(defclass detect_msgs::Yolo_Objects
  :super ros::object
  :slots (_header _yolo_objects ))

(defmethod detect_msgs::Yolo_Objects
  (:init
   (&key
    ((:header __header) (instance std_msgs::Header :init))
    ((:yolo_objects __yolo_objects) ())
    )
   (send-super :init)
   (setq _header __header)
   (setq _yolo_objects __yolo_objects)
   self)
  (:header
   (&rest __header)
   (if (keywordp (car __header))
       (send* _header __header)
     (progn
       (if __header (setq _header (car __header)))
       _header)))
  (:yolo_objects
   (&rest __yolo_objects)
   (if (keywordp (car __yolo_objects))
       (send* _yolo_objects __yolo_objects)
     (progn
       (if __yolo_objects (setq _yolo_objects (car __yolo_objects)))
       _yolo_objects)))
  (:serialization-length
   ()
   (+
    ;; std_msgs/Header _header
    (send _header :serialization-length)
    ;; detect_msgs/Objects[] _yolo_objects
    (apply #'+ (send-all _yolo_objects :serialization-length)) 4
    ))
  (:serialize
   (&optional strm)
   (let ((s (if strm strm
              (make-string-output-stream (send self :serialization-length)))))
     ;; std_msgs/Header _header
       (send _header :serialize s)
     ;; detect_msgs/Objects[] _yolo_objects
     (write-long (length _yolo_objects) s)
     (dolist (elem _yolo_objects)
       (send elem :serialize s)
       )
     ;;
     (if (null strm) (get-output-stream-string s))))
  (:deserialize
   (buf &optional (ptr- 0))
   ;; std_msgs/Header _header
     (send _header :deserialize buf ptr-) (incf ptr- (send _header :serialization-length))
   ;; detect_msgs/Objects[] _yolo_objects
   (let (n)
     (setq n (sys::peek buf ptr- :integer)) (incf ptr- 4)
     (setq _yolo_objects (let (r) (dotimes (i n) (push (instance detect_msgs::Objects :init) r)) r))
     (dolist (elem- _yolo_objects)
     (send elem- :deserialize buf ptr-) (incf ptr- (send elem- :serialization-length))
     ))
   ;;
   self)
  )

(setf (get detect_msgs::Yolo_Objects :md5sum-) "7f94689ed67d6f56bbe6b59436a3a900")
(setf (get detect_msgs::Yolo_Objects :datatype-) "detect_msgs/Yolo_Objects")
(setf (get detect_msgs::Yolo_Objects :definition-)
      "std_msgs/Header header
Objects[] yolo_objects

================================================================================
MSG: std_msgs/Header
# Standard metadata for higher-level stamped data types.
# This is generally used to communicate timestamped data 
# in a particular coordinate frame.
# 
# sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.sec: seconds (stamp_secs) since epoch (in Python the variable is called 'secs')
# * stamp.nsec: nanoseconds since stamp_secs (in Python the variable is called 'nsecs')
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
string frame_id

================================================================================
MSG: detect_msgs/Objects
int32 Class
int32 id
int32 x1
int32 x2
int32 y1
int32 y2

")



(provide :detect_msgs/Yolo_Objects "7f94689ed67d6f56bbe6b59436a3a900")


