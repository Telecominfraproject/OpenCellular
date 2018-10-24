
def append_unit_info(context, info):
    _generic_append_info(context, 'UNIT_INFO', info)

def _generic_append_info(context, export_name, info):
    SEPARATOR = '\n'
    present = getattr(context.exports, export_name, None)
    if present:
        setattr(context.exports, export_name, present + SEPARATOR + info)
    else:
        setattr(context.exports, export_name, info)
