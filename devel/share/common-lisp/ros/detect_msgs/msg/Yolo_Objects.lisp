; Auto-generated. Do not edit!


(cl:in-package detect_msgs-msg)


;//! \htmlinclude Yolo_Objects.msg.html

(cl:defclass <Yolo_Objects> (roslisp-msg-protocol:ros-message)
  ((header
    :reader header
    :initarg :header
    :type std_msgs-msg:Header
    :initform (cl:make-instance 'std_msgs-msg:Header))
   (yolo_objects
    :reader yolo_objects
    :initarg :yolo_objects
    :type (cl:vector detect_msgs-msg:Objects)
   :initform (cl:make-array 0 :element-type 'detect_msgs-msg:Objects :initial-element (cl:make-instance 'detect_msgs-msg:Objects))))
)

(cl:defclass Yolo_Objects (<Yolo_Objects>)
  ())

(cl:defmethod cl:initialize-instance :after ((m <Yolo_Objects>) cl:&rest args)
  (cl:declare (cl:ignorable args))
  (cl:unless (cl:typep m 'Yolo_Objects)
    (roslisp-msg-protocol:msg-deprecation-warning "using old message class name detect_msgs-msg:<Yolo_Objects> is deprecated: use detect_msgs-msg:Yolo_Objects instead.")))

(cl:ensure-generic-function 'header-val :lambda-list '(m))
(cl:defmethod header-val ((m <Yolo_Objects>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader detect_msgs-msg:header-val is deprecated.  Use detect_msgs-msg:header instead.")
  (header m))

(cl:ensure-generic-function 'yolo_objects-val :lambda-list '(m))
(cl:defmethod yolo_objects-val ((m <Yolo_Objects>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader detect_msgs-msg:yolo_objects-val is deprecated.  Use detect_msgs-msg:yolo_objects instead.")
  (yolo_objects m))
(cl:defmethod roslisp-msg-protocol:serialize ((msg <Yolo_Objects>) ostream)
  "Serializes a message object of type '<Yolo_Objects>"
  (roslisp-msg-protocol:serialize (cl:slot-value msg 'header) ostream)
  (cl:let ((__ros_arr_len (cl:length (cl:slot-value msg 'yolo_objects))))
    (cl:write-byte (cl:ldb (cl:byte 8 0) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) __ros_arr_len) ostream))
  (cl:map cl:nil #'(cl:lambda (ele) (roslisp-msg-protocol:serialize ele ostream))
   (cl:slot-value msg 'yolo_objects))
)
(cl:defmethod roslisp-msg-protocol:deserialize ((msg <Yolo_Objects>) istream)
  "Deserializes a message object of type '<Yolo_Objects>"
  (roslisp-msg-protocol:deserialize (cl:slot-value msg 'header) istream)
  (cl:let ((__ros_arr_len 0))
    (cl:setf (cl:ldb (cl:byte 8 0) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 8) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 16) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 24) __ros_arr_len) (cl:read-byte istream))
  (cl:setf (cl:slot-value msg 'yolo_objects) (cl:make-array __ros_arr_len))
  (cl:let ((vals (cl:slot-value msg 'yolo_objects)))
    (cl:dotimes (i __ros_arr_len)
    (cl:setf (cl:aref vals i) (cl:make-instance 'detect_msgs-msg:Objects))
  (roslisp-msg-protocol:deserialize (cl:aref vals i) istream))))
  msg
)
(cl:defmethod roslisp-msg-protocol:ros-datatype ((msg (cl:eql '<Yolo_Objects>)))
  "Returns string type for a message object of type '<Yolo_Objects>"
  "detect_msgs/Yolo_Objects")
(cl:defmethod roslisp-msg-protocol:ros-datatype ((msg (cl:eql 'Yolo_Objects)))
  "Returns string type for a message object of type 'Yolo_Objects"
  "detect_msgs/Yolo_Objects")
(cl:defmethod roslisp-msg-protocol:md5sum ((type (cl:eql '<Yolo_Objects>)))
  "Returns md5sum for a message object of type '<Yolo_Objects>"
  "7f94689ed67d6f56bbe6b59436a3a900")
(cl:defmethod roslisp-msg-protocol:md5sum ((type (cl:eql 'Yolo_Objects)))
  "Returns md5sum for a message object of type 'Yolo_Objects"
  "7f94689ed67d6f56bbe6b59436a3a900")
(cl:defmethod roslisp-msg-protocol:message-definition ((type (cl:eql '<Yolo_Objects>)))
  "Returns full string definition for message of type '<Yolo_Objects>"
  (cl:format cl:nil "std_msgs/Header header~%Objects[] yolo_objects~%~%================================================================================~%MSG: std_msgs/Header~%# Standard metadata for higher-level stamped data types.~%# This is generally used to communicate timestamped data ~%# in a particular coordinate frame.~%# ~%# sequence ID: consecutively increasing ID ~%uint32 seq~%#Two-integer timestamp that is expressed as:~%# * stamp.sec: seconds (stamp_secs) since epoch (in Python the variable is called 'secs')~%# * stamp.nsec: nanoseconds since stamp_secs (in Python the variable is called 'nsecs')~%# time-handling sugar is provided by the client library~%time stamp~%#Frame this data is associated with~%string frame_id~%~%================================================================================~%MSG: detect_msgs/Objects~%int32 Class~%int32 id~%int32 x1~%int32 x2~%int32 y1~%int32 y2~%~%~%"))
(cl:defmethod roslisp-msg-protocol:message-definition ((type (cl:eql 'Yolo_Objects)))
  "Returns full string definition for message of type 'Yolo_Objects"
  (cl:format cl:nil "std_msgs/Header header~%Objects[] yolo_objects~%~%================================================================================~%MSG: std_msgs/Header~%# Standard metadata for higher-level stamped data types.~%# This is generally used to communicate timestamped data ~%# in a particular coordinate frame.~%# ~%# sequence ID: consecutively increasing ID ~%uint32 seq~%#Two-integer timestamp that is expressed as:~%# * stamp.sec: seconds (stamp_secs) since epoch (in Python the variable is called 'secs')~%# * stamp.nsec: nanoseconds since stamp_secs (in Python the variable is called 'nsecs')~%# time-handling sugar is provided by the client library~%time stamp~%#Frame this data is associated with~%string frame_id~%~%================================================================================~%MSG: detect_msgs/Objects~%int32 Class~%int32 id~%int32 x1~%int32 x2~%int32 y1~%int32 y2~%~%~%"))
(cl:defmethod roslisp-msg-protocol:serialization-length ((msg <Yolo_Objects>))
  (cl:+ 0
     (roslisp-msg-protocol:serialization-length (cl:slot-value msg 'header))
     4 (cl:reduce #'cl:+ (cl:slot-value msg 'yolo_objects) :key #'(cl:lambda (ele) (cl:declare (cl:ignorable ele)) (cl:+ (roslisp-msg-protocol:serialization-length ele))))
))
(cl:defmethod roslisp-msg-protocol:ros-message-to-list ((msg <Yolo_Objects>))
  "Converts a ROS message object to a list"
  (cl:list 'Yolo_Objects
    (cl:cons ':header (header msg))
    (cl:cons ':yolo_objects (yolo_objects msg))
))
