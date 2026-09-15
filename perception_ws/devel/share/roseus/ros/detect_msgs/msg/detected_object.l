;; Auto-generated. Do not edit!


(when (boundp 'detect_msgs::detected_object)
  (if (not (find-package "DETECT_MSGS"))
    (make-package "DETECT_MSGS"))
  (shadow 'detected_object (find-package "DETECT_MSGS")))
(unless (find-package "DETECT_MSGS::DETECTED_OBJECT")
  (make-package "DETECT_MSGS::DETECTED_OBJECT"))

(in-package "ROS")
;;//! \htmlinclude detected_object.msg.html
(if (not (find-package "GEOMETRY_MSGS"))
  (ros::roseus-add-msgs "geometry_msgs"))


(defclass detect_msgs::detected_object
  :super ros::object
  :slots (_id _world_point ))

(defmethod detect_msgs::detected_object
  (:init
   (&key
    ((:id __id) 0)
    ((:world_point __world_point) (instance geometry_msgs::Pose :init))
    )
   (send-super :init)
   (setq _id (round __id))
   (setq _world_point __world_point)
   self)
  (:id
   (&optional __id)
   (if __id (setq _id __id)) _id)
  (:world_point
   (&rest __world_point)
   (if (keywordp (car __world_point))
       (send* _world_point __world_point)
     (progn
       (if __world_point (setq _world_point (car __world_point)))
       _world_point)))
  (:serialization-length
   ()
   (+
    ;; int64 _id
    8
    ;; geometry_msgs/Pose _world_point
    (send _world_point :serialization-length)
    ))
  (:serialize
   (&optional strm)
   (let ((s (if strm strm
              (make-string-output-stream (send self :serialization-length)))))
     ;; int64 _id
#+(or :alpha :irix6 :x86_64)
       (progn (sys::poke _id (send s :buffer) (send s :count) :long) (incf (stream-count s) 8))
#-(or :alpha :irix6 :x86_64)
       (cond ((and (class _id) (= (length (_id . bv)) 2)) ;; bignum
              (write-long (ash (elt (_id . bv) 0) 0) s)
              (write-long (ash (elt (_id . bv) 1) -1) s))
             ((and (class _id) (= (length (_id . bv)) 1)) ;; big1
              (write-long (elt (_id . bv) 0) s)
              (write-long (if (>= _id 0) 0 #xffffffff) s))
             (t                                         ;; integer
              (write-long _id s)(write-long (if (>= _id 0) 0 #xffffffff) s)))
     ;; geometry_msgs/Pose _world_point
       (send _world_point :serialize s)
     ;;
     (if (null strm) (get-output-stream-string s))))
  (:deserialize
   (buf &optional (ptr- 0))
   ;; int64 _id
#+(or :alpha :irix6 :x86_64)
      (setf _id (prog1 (sys::peek buf ptr- :long) (incf ptr- 8)))
#-(or :alpha :irix6 :x86_64)
      (setf _id (let ((b0 (prog1 (sys::peek buf ptr- :integer) (incf ptr- 4)))
                  (b1 (prog1 (sys::peek buf ptr- :integer) (incf ptr- 4))))
              (cond ((= b1 -1) b0)
                     ((and (= b1  0)
                           (<= lisp::most-negative-fixnum b0 lisp::most-positive-fixnum))
                      b0)
                    ((= b1  0) (make-instance bignum :size 1 :bv (integer-vector b0)))
                    (t (make-instance bignum :size 2 :bv (integer-vector b0 (ash b1 1)))))))
   ;; geometry_msgs/Pose _world_point
     (send _world_point :deserialize buf ptr-) (incf ptr- (send _world_point :serialization-length))
   ;;
   self)
  )

(setf (get detect_msgs::detected_object :md5sum-) "7a614a22096bf2e77defb774f827e23b")
(setf (get detect_msgs::detected_object :datatype-) "detect_msgs/detected_object")
(setf (get detect_msgs::detected_object :definition-)
      "int64 id
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



(provide :detect_msgs/detected_object "7a614a22096bf2e77defb774f827e23b")


