; Auto-generated. Do not edit!


(cl:in-package detect_msgs-msg)


;//! \htmlinclude detected_object.msg.html

(cl:defclass <detected_object> (roslisp-msg-protocol:ros-message)
  ((id
    :reader id
    :initarg :id
    :type cl:integer
    :initform 0)
   (world_point
    :reader world_point
    :initarg :world_point
    :type geometry_msgs-msg:Pose
    :initform (cl:make-instance 'geometry_msgs-msg:Pose)))
)

(cl:defclass detected_object (<detected_object>)
  ())

(cl:defmethod cl:initialize-instance :after ((m <detected_object>) cl:&rest args)
  (cl:declare (cl:ignorable args))
  (cl:unless (cl:typep m 'detected_object)
    (roslisp-msg-protocol:msg-deprecation-warning "using old message class name detect_msgs-msg:<detected_object> is deprecated: use detect_msgs-msg:detected_object instead.")))

(cl:ensure-generic-function 'id-val :lambda-list '(m))
(cl:defmethod id-val ((m <detected_object>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader detect_msgs-msg:id-val is deprecated.  Use detect_msgs-msg:id instead.")
  (id m))

(cl:ensure-generic-function 'world_point-val :lambda-list '(m))
(cl:defmethod world_point-val ((m <detected_object>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader detect_msgs-msg:world_point-val is deprecated.  Use detect_msgs-msg:world_point instead.")
  (world_point m))
(cl:defmethod roslisp-msg-protocol:serialize ((msg <detected_object>) ostream)
  "Serializes a message object of type '<detected_object>"
  (cl:let* ((signed (cl:slot-value msg 'id)) (unsigned (cl:if (cl:< signed 0) (cl:+ signed 18446744073709551616) signed)))
    (cl:write-byte (cl:ldb (cl:byte 8 0) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 32) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 40) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 48) unsigned) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 56) unsigned) ostream)
    )
  (roslisp-msg-protocol:serialize (cl:slot-value msg 'world_point) ostream)
)
(cl:defmethod roslisp-msg-protocol:deserialize ((msg <detected_object>) istream)
  "Deserializes a message object of type '<detected_object>"
    (cl:let ((unsigned 0))
      (cl:setf (cl:ldb (cl:byte 8 0) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 8) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 16) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 24) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 32) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 40) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 48) unsigned) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 56) unsigned) (cl:read-byte istream))
      (cl:setf (cl:slot-value msg 'id) (cl:if (cl:< unsigned 9223372036854775808) unsigned (cl:- unsigned 18446744073709551616))))
  (roslisp-msg-protocol:deserialize (cl:slot-value msg 'world_point) istream)
  msg
)
(cl:defmethod roslisp-msg-protocol:ros-datatype ((msg (cl:eql '<detected_object>)))
  "Returns string type for a message object of type '<detected_object>"
  "detect_msgs/detected_object")
(cl:defmethod roslisp-msg-protocol:ros-datatype ((msg (cl:eql 'detected_object)))
  "Returns string type for a message object of type 'detected_object"
  "detect_msgs/detected_object")
(cl:defmethod roslisp-msg-protocol:md5sum ((type (cl:eql '<detected_object>)))
  "Returns md5sum for a message object of type '<detected_object>"
  "7a614a22096bf2e77defb774f827e23b")
(cl:defmethod roslisp-msg-protocol:md5sum ((type (cl:eql 'detected_object)))
  "Returns md5sum for a message object of type 'detected_object"
  "7a614a22096bf2e77defb774f827e23b")
(cl:defmethod roslisp-msg-protocol:message-definition ((type (cl:eql '<detected_object>)))
  "Returns full string definition for message of type '<detected_object>"
  (cl:format cl:nil "int64 id~%geometry_msgs/Pose world_point~%================================================================================~%MSG: geometry_msgs/Pose~%# A representation of pose in free space, composed of position and orientation. ~%Point position~%Quaternion orientation~%~%================================================================================~%MSG: geometry_msgs/Point~%# This contains the position of a point in free space~%float64 x~%float64 y~%float64 z~%~%================================================================================~%MSG: geometry_msgs/Quaternion~%# This represents an orientation in free space in quaternion form.~%~%float64 x~%float64 y~%float64 z~%float64 w~%~%~%"))
(cl:defmethod roslisp-msg-protocol:message-definition ((type (cl:eql 'detected_object)))
  "Returns full string definition for message of type 'detected_object"
  (cl:format cl:nil "int64 id~%geometry_msgs/Pose world_point~%================================================================================~%MSG: geometry_msgs/Pose~%# A representation of pose in free space, composed of position and orientation. ~%Point position~%Quaternion orientation~%~%================================================================================~%MSG: geometry_msgs/Point~%# This contains the position of a point in free space~%float64 x~%float64 y~%float64 z~%~%================================================================================~%MSG: geometry_msgs/Quaternion~%# This represents an orientation in free space in quaternion form.~%~%float64 x~%float64 y~%float64 z~%float64 w~%~%~%"))
(cl:defmethod roslisp-msg-protocol:serialization-length ((msg <detected_object>))
  (cl:+ 0
     8
     (roslisp-msg-protocol:serialization-length (cl:slot-value msg 'world_point))
))
(cl:defmethod roslisp-msg-protocol:ros-message-to-list ((msg <detected_object>))
  "Converts a ROS message object to a list"
  (cl:list 'detected_object
    (cl:cons ':id (id msg))
    (cl:cons ':world_point (world_point msg))
))
