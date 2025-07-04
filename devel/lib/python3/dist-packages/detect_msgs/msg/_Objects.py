# This Python file uses the following encoding: utf-8
"""autogenerated by genpy from detect_msgs/Objects.msg. Do not edit."""
import codecs
import sys
python3 = True if sys.hexversion > 0x03000000 else False
import genpy
import struct


class Objects(genpy.Message):
  _md5sum = "0c3939f535e7f9cd3b8fb5460ead7a6b"
  _type = "detect_msgs/Objects"
  _has_header = False  # flag to mark the presence of a Header object
  _full_text = """int32 Class
int32 id
int32 x1
int32 x2
int32 y1
int32 y2
"""
  __slots__ = ['Class','id','x1','x2','y1','y2']
  _slot_types = ['int32','int32','int32','int32','int32','int32']

  def __init__(self, *args, **kwds):
    """
    Constructor. Any message fields that are implicitly/explicitly
    set to None will be assigned a default value. The recommend
    use is keyword arguments as this is more robust to future message
    changes.  You cannot mix in-order arguments and keyword arguments.

    The available fields are:
       Class,id,x1,x2,y1,y2

    :param args: complete set of field values, in .msg order
    :param kwds: use keyword arguments corresponding to message field names
    to set specific fields.
    """
    if args or kwds:
      super(Objects, self).__init__(*args, **kwds)
      # message fields cannot be None, assign default values for those that are
      if self.Class is None:
        self.Class = 0
      if self.id is None:
        self.id = 0
      if self.x1 is None:
        self.x1 = 0
      if self.x2 is None:
        self.x2 = 0
      if self.y1 is None:
        self.y1 = 0
      if self.y2 is None:
        self.y2 = 0
    else:
      self.Class = 0
      self.id = 0
      self.x1 = 0
      self.x2 = 0
      self.y1 = 0
      self.y2 = 0

  def _get_types(self):
    """
    internal API method
    """
    return self._slot_types

  def serialize(self, buff):
    """
    serialize message into buffer
    :param buff: buffer, ``StringIO``
    """
    try:
      _x = self
      buff.write(_get_struct_6i().pack(_x.Class, _x.id, _x.x1, _x.x2, _x.y1, _x.y2))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(locals().get('_x', self)))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(locals().get('_x', self)))))

  def deserialize(self, str):
    """
    unpack serialized message in str into this message instance
    :param str: byte array of serialized message, ``str``
    """
    if python3:
      codecs.lookup_error("rosmsg").msg_type = self._type
    try:
      end = 0
      _x = self
      start = end
      end += 24
      (_x.Class, _x.id, _x.x1, _x.x2, _x.y1, _x.y2,) = _get_struct_6i().unpack(str[start:end])
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e)  # most likely buffer underfill


  def serialize_numpy(self, buff, numpy):
    """
    serialize message with numpy array types into buffer
    :param buff: buffer, ``StringIO``
    :param numpy: numpy python module
    """
    try:
      _x = self
      buff.write(_get_struct_6i().pack(_x.Class, _x.id, _x.x1, _x.x2, _x.y1, _x.y2))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(locals().get('_x', self)))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(locals().get('_x', self)))))

  def deserialize_numpy(self, str, numpy):
    """
    unpack serialized message in str into this message instance using numpy for array types
    :param str: byte array of serialized message, ``str``
    :param numpy: numpy python module
    """
    if python3:
      codecs.lookup_error("rosmsg").msg_type = self._type
    try:
      end = 0
      _x = self
      start = end
      end += 24
      (_x.Class, _x.id, _x.x1, _x.x2, _x.y1, _x.y2,) = _get_struct_6i().unpack(str[start:end])
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e)  # most likely buffer underfill

_struct_I = genpy.struct_I
def _get_struct_I():
    global _struct_I
    return _struct_I
_struct_6i = None
def _get_struct_6i():
    global _struct_6i
    if _struct_6i is None:
        _struct_6i = struct.Struct("<6i")
    return _struct_6i
