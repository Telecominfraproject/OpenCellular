def to_unicode(obj):
    unicode_obj = obj
    try:
        unicode_obj = unicode(obj, 'ascii', 'ignore')
    # except ValueError:
    #     unicode_obj = unicode(obj, 'utf-8')
    except TypeError:
        pass
    return unicode_obj
