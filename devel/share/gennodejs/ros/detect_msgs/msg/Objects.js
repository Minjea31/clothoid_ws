// Auto-generated. Do not edit!

// (in-package detect_msgs.msg)


"use strict";

const _serializer = _ros_msg_utils.Serialize;
const _arraySerializer = _serializer.Array;
const _deserializer = _ros_msg_utils.Deserialize;
const _arrayDeserializer = _deserializer.Array;
const _finder = _ros_msg_utils.Find;
const _getByteLength = _ros_msg_utils.getByteLength;

//-----------------------------------------------------------

class Objects {
  constructor(initObj={}) {
    if (initObj === null) {
      // initObj === null is a special case for deserialization where we don't initialize fields
      this.Class = null;
      this.id = null;
      this.x1 = null;
      this.x2 = null;
      this.y1 = null;
      this.y2 = null;
    }
    else {
      if (initObj.hasOwnProperty('Class')) {
        this.Class = initObj.Class
      }
      else {
        this.Class = 0;
      }
      if (initObj.hasOwnProperty('id')) {
        this.id = initObj.id
      }
      else {
        this.id = 0;
      }
      if (initObj.hasOwnProperty('x1')) {
        this.x1 = initObj.x1
      }
      else {
        this.x1 = 0;
      }
      if (initObj.hasOwnProperty('x2')) {
        this.x2 = initObj.x2
      }
      else {
        this.x2 = 0;
      }
      if (initObj.hasOwnProperty('y1')) {
        this.y1 = initObj.y1
      }
      else {
        this.y1 = 0;
      }
      if (initObj.hasOwnProperty('y2')) {
        this.y2 = initObj.y2
      }
      else {
        this.y2 = 0;
      }
    }
  }

  static serialize(obj, buffer, bufferOffset) {
    // Serializes a message object of type Objects
    // Serialize message field [Class]
    bufferOffset = _serializer.int32(obj.Class, buffer, bufferOffset);
    // Serialize message field [id]
    bufferOffset = _serializer.int32(obj.id, buffer, bufferOffset);
    // Serialize message field [x1]
    bufferOffset = _serializer.int32(obj.x1, buffer, bufferOffset);
    // Serialize message field [x2]
    bufferOffset = _serializer.int32(obj.x2, buffer, bufferOffset);
    // Serialize message field [y1]
    bufferOffset = _serializer.int32(obj.y1, buffer, bufferOffset);
    // Serialize message field [y2]
    bufferOffset = _serializer.int32(obj.y2, buffer, bufferOffset);
    return bufferOffset;
  }

  static deserialize(buffer, bufferOffset=[0]) {
    //deserializes a message object of type Objects
    let len;
    let data = new Objects(null);
    // Deserialize message field [Class]
    data.Class = _deserializer.int32(buffer, bufferOffset);
    // Deserialize message field [id]
    data.id = _deserializer.int32(buffer, bufferOffset);
    // Deserialize message field [x1]
    data.x1 = _deserializer.int32(buffer, bufferOffset);
    // Deserialize message field [x2]
    data.x2 = _deserializer.int32(buffer, bufferOffset);
    // Deserialize message field [y1]
    data.y1 = _deserializer.int32(buffer, bufferOffset);
    // Deserialize message field [y2]
    data.y2 = _deserializer.int32(buffer, bufferOffset);
    return data;
  }

  static getMessageSize(object) {
    return 24;
  }

  static datatype() {
    // Returns string type for a message object
    return 'detect_msgs/Objects';
  }

  static md5sum() {
    //Returns md5sum for a message object
    return '0c3939f535e7f9cd3b8fb5460ead7a6b';
  }

  static messageDefinition() {
    // Returns full string definition for message
    return `
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
    const resolved = new Objects(null);
    if (msg.Class !== undefined) {
      resolved.Class = msg.Class;
    }
    else {
      resolved.Class = 0
    }

    if (msg.id !== undefined) {
      resolved.id = msg.id;
    }
    else {
      resolved.id = 0
    }

    if (msg.x1 !== undefined) {
      resolved.x1 = msg.x1;
    }
    else {
      resolved.x1 = 0
    }

    if (msg.x2 !== undefined) {
      resolved.x2 = msg.x2;
    }
    else {
      resolved.x2 = 0
    }

    if (msg.y1 !== undefined) {
      resolved.y1 = msg.y1;
    }
    else {
      resolved.y1 = 0
    }

    if (msg.y2 !== undefined) {
      resolved.y2 = msg.y2;
    }
    else {
      resolved.y2 = 0
    }

    return resolved;
    }
};

module.exports = Objects;
