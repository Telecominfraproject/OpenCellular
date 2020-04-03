class PawscJson:

    def __init__(self,ver):
        self.Msg = {}
        self.Msg['jsonrpc'] = ver
    
    def MethodSet(self,mthd):
        self.Msg['method'] = mthd
       
    def ParamSet(self, param):
        self.Msg['params'] = param
      
    def IDSet(self, id):
        self.Msg['id'] = id
    
    def ErrorSet(self, param):
        self.Msg['error'] = param

    def FieldSet(self,mthd,param,id):
        self.Msg['id'] = id
        self.Msg['params'] = param
        self.Msg['method'] = mthd
       
    
    def Get(self):
        return self.Msg


