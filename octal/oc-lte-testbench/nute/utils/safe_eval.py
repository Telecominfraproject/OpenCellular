SAFE_SYMBOLS = ["list", "dict", "tuple", "set", "long", "float", "object",
                "bool", "callable", "True", "False", "dir",
                "frozenset", "getattr", "hasattr", "abs", "cmp", "complex",
                "divmod", "id", "pow", "round", "slice", "vars",
                "hash", "hex", "int", "isinstance", "issubclass", "len",
                "map", "filter", "max", "min", "oct", "chr", "ord", "range",
                "reduce", "repr", "str", "type", "zip", "xrange", "None",
                "Exception", "KeyboardInterrupt"]
# Also add the standard exceptions
__bi = __builtins__
if type(__bi) is not dict:
    __bi = __bi.__dict__
for k in __bi:
    if k.endswith("Error") or k.endswith("Warning"):
        SAFE_SYMBOLS.append(k)
del __bi

def safe_eval(s, additional_symbols=dict(), file_name='(string)'):
    if not s:
        return None
    byteCode = compile(s, file_name, 'eval')
    # Setup the local and global dictionaries of the execution
    # environment for __TheFunction__
    bis   = dict() # builtins
    globs = dict()
    locs  = dict()

    # Setup a standard-compatible python environment
    bis["locals"]  = lambda: locs
    bis["globals"] = lambda: globs
    globs["__builtins__"] = bis
    globs["__name__"] = "SUBENV"

    # Determine how the __builtins__ dictionary should be accessed
    bi_dict = get_builtins()
    # Include the safe symbols
    for k in SAFE_SYMBOLS:
        # try from current locals
        try:
            locs[k] = locals()[k]
            continue
        except KeyError:
            pass
        # Try from globals
        try:
            globs[k] = globals()[k]
            continue
        except KeyError:
            pass
        # Try from builtins
        try:
            bis[k] = bi_dict[k]
        except KeyError:
            # Symbol not available anywhere: silently ignored
            pass

    # Include the symbols added by the caller, in the globals dictionary
    globs.update(additional_symbols)

    # Finally execute the def __TheFunction__ statement:
    return eval(byteCode, globs, locs)

def get_builtins():
    if type(__builtins__) is dict:
        bi_dict = __builtins__
    else:
        bi_dict = __builtins__.__dict__
    return bi_dict


def replace_names(s, names={}, ignore_names=[], tolerate_missing_names=False):
    import ast
    import astor
    from ast import NodeTransformer

    class EvalVisitor(NodeTransformer):
        def __init__(self, replace={}, ignore=[]):
            self._namespace = replace
            self._ignore = ignore
            self._ignore = self._ignore + get_builtins().keys()

        def visit_Name(self, node):
            if node.id in self._ignore:
                return node

            try:
                value = ast.Str(self._namespace[node.id])

                return ast.copy_location(ast.Expr(value), node)
            except KeyError:
                if tolerate_missing_names:
                    return node
                else:
                    raise

    v = EvalVisitor(replace=names, ignore=ignore_names)
    compiled = ast.parse(s)
    v.visit(compiled)

    return astor.to_source(compiled).replace('\r', '').replace('\n', '')
