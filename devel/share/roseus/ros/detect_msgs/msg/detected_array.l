;; Auto-generated. Do not edit!


(when (boundp 'detect_msgs::detected_array)
  (if (not (find-package "DETECT_MSGS"))
    (make-package "DETECT_MSGS"))
  (shadow 'detected_array (find-package "DETECT_MSGS")))
(unless (find-package "DETECT_MSGS::DETECTED_ARRAY")
  (make-package "DETECT_MSGS::DETECTED_ARRAY"))

(in-package "ROS")
;;//! \htmlinclude detected_array.msg.html
(if (not (find-package "STD_MSGS"))
  (ros::roseus-add-msgs "std_msgs"))


(defclass detect_msgs::detected_array
  :super ros::object
  :slots (_header _objects ))

(defmethod detect_msgs::detected_array
  (:init
   (&key
    ((:header __header) (instance std_msgs::Header :init))
    ((:objects __objects) ())
    )
   (send-super :init)
   (setq _header __header)
   (setq _objects __objects)
   self)
  (:header
   (&rest __header)
   (if (keywordp (car __header))
       (send* _header __header)
     (progn
       (if __header (setq _header (car __header)))
       _header)))
  (:objects
   (&rest __objects)
   (if (keywordp (car __objects))
       (send* _objects __objects)
     (progn
       (if __objects (setq _objects (car __objects)))
       _objects)))
  (:serialization-length
   ()
   (+
    ;; std_msgs/Header _header
    (send _header :serialization-length)
    ;; detect_msgs/detected_object[] _objects
    (apply #'+ (send-all _objects :serialization-length)) 4
    ))
  (:serialize
   (&optional strm)
   (let ((s (if strm strm
              (make-string-output-stream (send self :serialization-length)))))
     ;; std_msgs/Header _header
       (send _header :serialize s)
     ;; detect_msgs/detected_object[] _objects
     (write-long (length _objects) s)
     (dolist (elem _objects)
       (send elem :serialize s)
       )
     ;;
     (if (null strm) (get-output-stream-string s))))
  (:deserialize
   (buf &optional (ptr- 0))
   ;; std_msgs/Header _header
     (send _header :deserialize buf ptr-) (incf ptr- (send _header :serialization-length))
   ;; detect_msgs/detected_object[] _objects
   (let (n)
     (setq n (sys::peek buf ptr- :integer)) (incf ptr- 4)
     (setq _objects (let (r) (dotimes (i n) (push (instance detect_msgs::detected_object :init) r)) r))
     (dolist (elem- _objects)
     (send elem- :deserialize buf ptr-) (incf ptr- (send elem- :serialization-length))
     ))
   ;;
   self)
  )

(setf (get detect_msgs::detected_array :md5sum-) "5e13babe394ae7d5a46bd763bac5f5b3")
(setf (get detect_msgs::detected_array :datatype-) "detect_msgs/detected_array")
(setf (get detect_msgs::detected_array :definition-)
      "std_msgs/Header header
detected_object[] objects
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
MSG: detect_msgs/detected_object
int64 id
geometry_msgs/Pose world_point
================================================================================
MSG: geometry_msgs/Pose
# A representation of pose in free space, composed of position and orientation. 
Point position
Quaternion orientation

================================================================================
MSG: geometry_msgs/Point
# This contains the position of a point in free space
float64 x
float64 y
float64 z

================================================================================
MSG: geometry_msgs/Quaternion
# This represents an orientation in free space in quaternion form.

float64 x
float64 y
float64 z
float64 w

")



(provide :detect_msgs/detected_array "5e13babe394ae7d5a46bd763bac5f5b3")


