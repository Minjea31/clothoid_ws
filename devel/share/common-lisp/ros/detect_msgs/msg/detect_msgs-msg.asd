
(cl:in-package :asdf)

(defsystem "detect_msgs-msg"
  :depends-on (:roslisp-msg-protocol :roslisp-utils :geometry_msgs-msg
               :std_msgs-msg
)
  :components ((:file "_package")
    (:file "Objects" :depends-on ("_package_Objects"))
    (:file "_package_Objects" :depends-on ("_package"))
    (:file "Yolo_Objects" :depends-on ("_package_Yolo_Objects"))
    (:file "_package_Yolo_Objects" :depends-on ("_package"))
    (:file "detected_array" :depends-on ("_package_detected_array"))
    (:file "_package_detected_array" :depends-on ("_package"))
    (:file "detected_object" :depends-on ("_package_detected_object"))
    (:file "_package_detected_object" :depends-on ("_package"))
  ))