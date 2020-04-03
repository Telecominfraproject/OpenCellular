#from base.src import constants
#from base.src.arfcn import FreqRangeToArfcnRange
#from base.src.calculation_engine import getSpectrumGLSD
#from base.src.cellularSpectrum import convertCellular



from base.src.AvailableSpectrumRequest import AvailSpecReq

# Experiments
# res = getSpectrumGLSD('CSIR',-34,18,'AGL',3)
# res = getSpectrumGLSD('CSIR',-34,18,'AGL',50)

# Beverly Hills
# res = getSpectrumGLSD('CSIR',-34.147344,18.360223,'AGL',5)

# David House
# res = getSpectrumGLSD('CSIR',-34.127162,18.427468,'AGL',2)

# Philipstown

# res = getSpectrumGLSD('CSIR',-30.436547,24.472518,'AGL',30)

# Mankosi
# res = getSpectrumGLSD('CSIR',-31.910233,29.170580,'AGL',5)

#resCellular  = convertCellular([{'channelWidthHz': 5000000},{'channelWidthHz': 10000000}], res, 'LTE', 'Band20')

#print (resCellular)

res = AvailSpecReq({'test': 1})
print()
print()
print(res)