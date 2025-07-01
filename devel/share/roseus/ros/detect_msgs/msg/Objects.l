;; Auto-generated. Do not edit!


(when (boundp 'detect_msgs::Objects)
  (if (not (find-package "DETECT_MSGS"))
    (make-package "DETECT_MSGS"))
  (shadow 'Objects (find-package "DETECT_MSGS")))
(unless (find-package "DETECT_MSGS::OBJECTS")
  (make-package "DETECT_MSGS::OBJECTS"))

(in-package "ROS")
;;//! \htmlinclude Objects.msg.html


(defclass detect_msgs::Objects
  :super ros::object
  :slots (_Class _id _x1 _x2 _y1 _y2 ))

(defmethod detect_msgs::Objects
  (:init
   (&key
    ((:Class __Class) 0)
    ((:id __id) 0)
    ((:x1 __x1) 0)
    ((:x2 __x2) 0)
    ((:y1 __y1) 0)
    ((:y2 __y2) 0)
    )
   (send-super :init)
   (setq _Class (round __Class))
   (setq _id (round __id))
   (setq _x1 (round __x1))
   (setq _x2 (round __x2))
   (setq _y1 (round __y1))
   (setq _y2 (round __y2))
   self)
  (:Class
   (&optional __Class)
   (if __Class (setq _Class __Class)) _Class)
  (:id
   (&optional __id)
   (if __id (setq _id __id)) _id)
  (:x1
   (&optional __x1)
   (if __x1 (setq _x1 __x1)) _x1)
  (:x2
   (&optional __x2)
   (if __x2 (setq _x2 __x2)) _x2)
  (:y1
   (&optional __y1)
   (if __y1 (setq _y1 __y1)) _y1)
  (:y2
   (&optional __y2)
   (if __y2 (setq _y2 __y2)) _y2)
  (:serialization-length
   ()
   (+
    ;; int32 _Class
    4
    ;; int32 _id
    4
    ;; int32 _x1
    4
    ;; int32 _x2
    4
    ;; int32 _y1
    4
    ;; int32 _y2
    4
    ))
  (:serialize
   (&optional strm)
   (let ((s (if strm strm
              (make-string-output-stream (send self :serialization-length)))))
     ;; int32 _Class
       (write-long _Class s)
     ;; int32 _id
       (write-long _id s)
     ;; int32 _x1
       (write-long _x1 s)
     ;; int32 _x2
       (write-long _x2 s)
     ;; int32 _y1
       (write-long _y1 s)
     ;; int32 _y2
       (write-long _y2 s)
     ;;
     (if (null strm) (get-output-stream-string s))))
  (:deserialize
   (buf &optional (ptr- 0))
   ;; int32 _Class
     (setq _Class (sys::peek buf ptr- :integer)) (incf ptr- 4)
   ;; int32 _id
     (setq _id (sys::peek buf ptr- :integer)) (incf ptr- 4)
   ;; int32 _x1
     (setq _x1 (sys::peek buf ptr- :integer)) (incf ptr- 4)
   ;; int32 _x2
     (setq _x2 (sys::peek buf ptr- :integer)) (incf ptr- 4)
   ;; int32 _y1
     (setq _y1 (sys::peek buf ptr- :integer)) (incf ptr- 4)
   ;; int32 _y2
     (setq _y2 (sys::peek buf ptr- :integer)) (incf ptr- 4)
   ;;
   self)
  )

(setf (get detect_msgs::Objects :md5sum-) "0c3939f535e7f9cd3b8fb5460ead7a6b")
(setf (get detect_msgs::Objects :datatype-) "detect_msgs/Objects")
(setf (get detect_msgs::Objects :definition-)
      "int32 Class
int32 id
int32 x1
int32 x2
int32 y1
int32 y2

")



(provide :detect_msgs/Objects "0c3939f535e7f9cd3b8fb5460ead7a6b")


