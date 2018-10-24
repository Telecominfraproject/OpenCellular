from collections import Mapping
from anytree import NodeMixin
from ini_loader import EmptyObject
import collections

def observe_obj_setattr(obj, callback):
    func_type = type(obj.__setattr__)
    def set_attr(self, name, value):
        callback(name, value)
        super(obj.__class__, self).__setattr__(name, value)

    obj.__setattr__ = set_attr.__get__(obj, obj.__class__)

def serialize(orig_obj, lambda_fn, accepted_exceptions=(AttributeError,)):
    try:
        obj = lambda_fn(orig_obj)
    except Exception as e:
        if isinstance(e, accepted_exceptions):
            obj = orig_obj
        else:
            raise

    if isinstance(obj, collections.Sequence) and not isinstance(obj, basestring):
        serialized = [serialize(item, lambda_fn) for item in obj]
    elif isinstance(obj, collections.Mapping):
        serialized = {key: serialize(value, lambda_fn) for (key,value) in obj.items()}
    else:
        serialized = obj

    return serialized

class EmptyObjectObserver(object):
    def __init__(self, *args, **kwargs):
        self.observers = []
        self.observed = self._create_observed(*args, **kwargs)

    def _create_observed(outer_self, *args, **kwargs):
        class _ObservedEmptyObject(EmptyObject):
            def __setattr__(self, name, value):
                super(_ObservedEmptyObject, self).__setattr__(name, value)
                outer_self.callback(name, value)

        return _ObservedEmptyObject(*args, **kwargs)

    def callback(self, name, value):
        for observer in self.observers:
            observer(name, value)

    def observe(self, callback):
        self.observers.append(callback)

    def get_observed(self):
        return self.observed

class Event(object):
    pass

class Observable(object):
    def __init__(self):
        self.callbacks = []

    def subscribe(self, callback):
        self.callbacks.append(callback)

    def fire(self, **attrs):
        e = Event()
        e.source = self
        for k, v in attrs.iteritems():
            setattr(e, k, v)
        for fn in self.callbacks:
            fn(e)


class DataTransferObject(Observable, Mapping):
    """
    An object used for representation and transfer of object over a network.
    The data is in a dict instance.
        - Can only contain basic python types
        - Can have arbritrarly deep nests
        - Each object which is in a dictionnary can be updated individually, as
            long as all objects leading to it are dictionnaries also.

    The objective of this object is to isolate independant parts of the representation
    in order to update those parts individually while maintaining the state for
    new clients.

    Example:
        dto = DataTransferObject()
        dto.dict =
        {
        'layer0_0' : 'A',
        'layer0_1' : 'B',
        'layer0_2' : {
            'layer1_0' : 'C',
            'layer1_1' : 'D'
            }
        }
        dto.update(path=['layer0_0'], data= 'newA')
        dto.update(path=['layer0_2', 'layer1_0'], data= 'newC')
        dto.update(path=[], data=newRootDict)

    Observable events:
        DTOUpdate
        :param source: Refers to this DTO instance.
        :param path: The path to update.
        :param content: The content of the update.

    """
    def __init__(self, event_sub_path=[]):
        Mapping.__init__(self)
        Observable.__init__(self)
        self._event_sub_path = event_sub_path

    def _get_dict(self):
        try:
            d = self._dict
        except AttributeError:
            d = None
        if d is None:
            self._dict = {}
            d = self._dict
        return d

    def __getitem__(self, key):
        d = self._get_dict()
        assert d is not self
        return d[key]

    def __setitem__(self, key, value):
        d = self._get_dict()
        assert d is not self
        d[key] = value

    def __delitem__(self, key):
        d = self._get_dict()
        del d[key]

    def __iter__(self):
        return self._get_dict().__iter__()

    def __len__(self):
        return self._get_dict().__len__()

    def pop(self, *args, **kwargs):
        #Exact support of underlying dict pop function
        return self._get_dict().pop(*args, **kwargs)

    def update(self, data=None, path=[], op='update'):
        """
        :param path: An array of dictionnary indexes indicating the path
            of the object to update.
            An empty array indicates the root dictionnary. This is the default.
        :param data: The content to update.
        :param op: 'set', 'add', 'update'
        """

        try:
            current_data = self.get_path(path)
        except KeyError:
            current_data = None

        if op == 'set':
            self._set_path(path, data)
        elif op == 'add':
            self._set_path(path, current_data + data)
        elif op == 'update':
            try:
                current_data = self.deep_update(current_data, data)
                self._set_path(path, current_data)
            except AttributeError:
                self._set_path(path, data)

        self.fire_update_event(path, data=data, op=op)

    def set_sub_path(self, sub_path=[]):
        self._event_sub_path = sub_path

    def deep_update(self, d, u):
        if d is None:
            return u
        for k, v in u.iteritems():
            if isinstance(v, collections.Mapping) and not isinstance(v, DataTransferObject):
                d[k] = self.deep_update(d.get(k, {}), v)
            else:
                d[k] = v
        return d

    def _set_path(self, path, value):
        _dict = self._get_dict()
        if path:
            for p in path[:-1]:
                _dict = _dict.setdefault(p, {})
            _dict[path[-1]] = value
        else:
            self._dict = value

    def get_path(self, path=[]):
        _dict = self._get_dict()
        for p in path:
            if p:
                _dict = _dict[p]
        return _dict

    def fire_update_event(self, path=[], data=None, op='set'):
        if data is None:
            content = self.get_path(path)
        else:
            content = data

        try: content = serialize(content, lambda obj: obj.serialize(), accepted_exceptions=(AttributeError,))
        except: pass
        self.fire(path=self._event_sub_path + path, content=content, op=op)

    def observable_at_path(self, obj, path):
        def callback(key, value):
            self.update(data={key:value}, path=path)

        obj.observe(callback)
