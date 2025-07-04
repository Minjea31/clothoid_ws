
(cl:in-package :asdf)

(defsystem "detect_msgs-msg"
  :depends-on (:roslisp-msg-protocol :roslisp-utils :std_msgs-msg
)
  :components ((:file "_package")
    (:file "Objects" :depends-on ("_package_Objects"))
    (:file "_package_Objects" :depends-on ("_package"))
    (:file "Yolo_Objects" :depends-on ("_package_Yolo_Objects"))
    (:file "_package_Yolo_Objects" :depends-on ("_package"))
  ))