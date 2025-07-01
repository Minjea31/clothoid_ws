// Auto-generated. Do not edit!

// (in-package detect_msgs.msg)


"use strict";

const _serializer = _ros_msg_utils.Serialize;
const _arraySerializer = _serializer.Array;
const _deserializer = _ros_msg_utils.Deserialize;
const _arrayDeserializer = _deserializer.Array;
const _finder = _ros_msg_utils.Find;
const _getByteLength = _ros_msg_utils.getByteLength;
let Objects = require('./Objects.js');
let std_msgs = _finder('std_msgs');

//-----------------------------------------------------------

class Yolo_Objects {
  constructor(initObj={}) {
    if (initObj === null) {
      // initObj === null is a special case for deserialization where we don't initialize fields
      this.header = null;
      this.yolo_objects = null;
    }
    else {
      if (initObj.hasOwnProperty('header')) {
        this.header = initObj.header
      }
      else {
        this.header = new std_msgs.msg.Header();
      }
      if (initObj.hasOwnProperty('yolo_objects')) {
        this.yolo_objects = initObj.yolo_objects
      }
      else {
        this.yolo_objects = [];
      }
    }
  }

  static serialize(obj, buffer, bufferOffset) {
    // Serializes a message object of type Yolo_Objects
    // Serialize message field [header]
    bufferOffset = std_msgs.msg.Header.serialize(obj.header, buffer, bufferOffset);
    // Serialize message field [yolo_objects]
    // Serialize the length for message field [yolo_objects]
    bufferOffset = _serializer.uint32(obj.yolo_objects.length, buffer, bufferOffset);
    obj.yolo_objects.forEach((val) => {
      bufferOffset = Objects.serialize(val, buffer, bufferOffset);
    });
    return bufferOffset;
  }

  static deserialize(buffer, bufferOffset=[0]) {
    //deserializes a message object of type Yolo_Objects
    let len;
    let data = new Yolo_Objects(null);
    // Deserialize message field [header]
    data.header = std_msgs.msg.Header.deserialize(buffer, bufferOffset);
    // Deserialize message field [yolo_objects]
    // Deserialize array length for message field [yolo_objects]
    len = _deserializer.uint32(buffer, bufferOffset);
    data.yolo_objects = new Array(len);
    for (let i = 0; i < len; ++i) {
      data.yolo_objects[i] = Objects.deserialize(buffer, bufferOffset)
    }
    return data;
  }

  static getMessageSize(object) {
    let length = 0;
    length += std_msgs.msg.Header.getMessageSize(object.header);
    length += 24 * object.yolo_objects.length;
    return length + 4;
  }

  static datatype() {
    // Returns string type for a message object
    return 'detect_msgs/Yolo_Objects';
  }

  static md5sum() {
    //Returns md5sum for a message object
    return '7f94689ed67d6f56bbe6b59436a3a900';
  }

  static messageDefinition() {
    // Returns full string definition for message
    return `
    std_msgs/Header header
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
    
    `;
  }

  static Resolve(msg) {
    // deep-construct a valid message object instance of whatever was passed in
    if (typeof msg !== 'object' || msg === null) {
      msg = {};
    }
    const resolved = new Yolo_Objects(null);
    if (msg.header !== undefined) {
      resolved.header = std_msgs.msg.Header.Resolve(msg.header)
    }
    else {
      resolved.header = new std_msgs.msg.Header()
    }

    if (msg.yolo_objects !== undefined) {
      resolved.yolo_objects = new Array(msg.yolo_objects.length);
      for (let i = 0; i < resolved.yolo_objects.length; ++i) {
        resolved.yolo_objects[i] = Objects.Resolve(msg.yolo_objects[i]);
      }
    }
    else {
      resolved.yolo_objects = []
    }

    return resolved;
    }
};

module.exports = Yolo_Objects;
