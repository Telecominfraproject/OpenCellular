#!/usr/bin/python

"""
Fdownlink = FDL_Low + 0.1 (NDL - NDL_Offset) 
Fuplink = FUL_Low + 0.1 (NUL - NUL_Offset) 

where:
NDL is downlink EARFCN
NUL is uplink EARFCN
NDL_Offset is offset used to calculate downlink EARFCN
NUL_Offset is offset used to calculate uplink EARFCN
"""
class EutraEarfcn():
    
    def __init__(self):
        
        self.eutra_arr = {1:[2110, 2140, 2170, 0, 1920, 1950, 1980, 18000], 
                          2:[1930, 1960, 1990, 600, 1850, 1880, 1910, 18600], 
                          3:[1805, 1842.5, 1880, 1200, 1710, 1747.5, 1785, 19200], 
                          4:[2110, 2132.5, 2155, 1950, 1710, 1732.5, 1755, 19950], 
                          5:[869, 881.5, 894, 2400, 824, 836.5, 849, 20400], 
                          6:[875, 880, 885, 2650, 830, 835, 840, 20650], 
                          7:[2620, 2655, 2690, 2750, 2500, 2535, 2570, 20750], 
                          8:[925, 942.5, 960, 3450, 880, 897.5, 915, 21450], 
                          9:[1844.9, 1862.4, 1879.9, 3800, 1749.9, 1767.4, 1784.9, 21800],
                          10:[2110, 2140, 2170, 4150, 1710, 1740, 1770, 22150], 
                          11:[1475.9, 1485.9, 1495.9, 4750, 1427.9, 1437.9, 1447.9, 22750], 
                          12:[729, 737.5, 746, 5010, 699, 707.5, 716, 23010], 
                          13:[746, 751, 756, 5180, 777, 782, 787, 23180], 
                          14:[758, 763, 768, 5280, 788, 793, 798, 23280], 
                          17:[734, 740, 746, 5730, 704, 710, 716, 23730], 
                          18:[860, 867.5, 875, 5850, 815, 822.5, 830, 23850],
                          19:[875, 882.5, 890, 6000, 830, 837.5, 845, 24000], 
                          20:[791, 806, 821, 6150, 832, 847, 862, 24150], 
                          21:[1495.9, 1503.4, 1510.9, 6450, 1447.9, 1455.4, 1462.9, 24450], 
                          22:[3510, 3550, 3590, 6600, 3410, 3450, 3490, 24600], 
                          23:[2180, 2190, 2200, 7500, 2000, 2010, 2020, 25500], 
                          24:[1525, 1542, 1559, 7700, 1626.5, 1643.5, 1660,5, 25700], 
                          25:[1930, 1962.5, 1995, 8040, 1850, 1882.5, 1915, 26040],
                          26:[859, 876.5, 894, 8690, 814, 831.5, 849, 26690], 
                          27:[852, 860.5, 869, 9040, 807, 815.5, 824, 27040], 
                          28:[758, 780.5, 803, 9210, 703, 725.5, 748, 27210], 
                          30:[2350, 2355, 2360, 9770, 2305, 2310, 2315, 27660], 
                          31:[462.5, 465, 467.5, 9870, 452.5, 455, 457.5, 27760], 
                          33:[1900, 1910, 1920, 36000, 1900, 1910, 1920, 36000], 
                          34:[2010, 2017.5, 2025, 36200, 2010, 2017.5, 2025, 36200], 
                          35:[1850, 1880, 1910, 36350, 1850, 1880, 1910, 36350], 
                          36:[1930, 1960, 1990, 36950, 1930, 1960, 1990, 36950], 
                          37:[1910, 1920, 1930, 37550, 1910, 1920, 1930, 37550], 
                          38:[2570, 2595, 2620, 37750, 2570, 2595, 2620, 37750],
                          39:[1880, 1900, 1920, 38250, 1880, 1900, 1920, 38250], 
                          40:[2300, 2350, 2400, 38650, 2300, 2350, 2400, 38650], 
                          41:[2496, 2593, 2690, 39650, 2496, 2593, 2690, 39650], 
                          42:[3400, 3500, 3600, 41590, 3400, 3500, 3600, 41590], 
                          43:[3600, 3700, 3800, 43590, 3600, 3700, 3800, 43590], 
                          44:[703, 753, 803, 45590, 703, 753, 803, 45590]}
        
    
    def dl_freq2earfcn(self, band, freq):
        
        earfcn = -1
        binf = self.eutra_arr[int(band)]
        
        fmin = binf[0]
        fmax = binf[2]
        if (float(freq) < fmin) or (float(freq) > fmax):
            print "frequency not in this band"
            #print "band=" + str(band) + " fmin=" + str(fmin) + " fmax=" + str(fmax)
        else:
            earfcn = int(10*(float(freq) - binf[0]) + binf[3])
    
        return earfcn
    
    def ul_freq2earfcn(self, band, freq):
        
        earfcn = -1
        binf = self.eutra_arr.get(int(band))
        
        fmin = binf[4]
        fmax = binf[6]
        if (float(freq) < fmin) or (float(freq) > fmax):
            print "frequency not in this band"
        else:
            earfcn = int(10*(float(freq) - binf[4]) + binf[7])
            
        return earfcn
    
    def dl_earfcn2freq(self, earfcn):
    
        freq = 0
        for _, elem in self.eutra_arr.items():
            
            emin = elem[3]
            emax = elem[3] + 10*(elem[2] - elem[0]) - 1
            
            if (int(earfcn) > emin) and (int(earfcn) < emax):
                freq = elem[0] + 0.1*(int(earfcn) - elem[3])
            
        return freq
    
    def ul_earfcn2freq(self, earfcn):
    
        freq = 0
        for _, elem in self.eutra_arr.items():
            
            emin = elem[7]
            emax = elem[7] + 10*(elem[6] - elem[4]) - 1
            
            if (int(earfcn) > emin) and (int(earfcn) < emax):
                freq = elem[4] + 0.1*(int(earfcn) - elem[7])
            
        return freq
    
    def get_middle_dl_earfcn(self, band):
        
        binf = self.eutra_arr[int(band)]
        earfcn = 5*(binf[2] - binf[0]) + binf[3]
        return earfcn
    
    def get_middle_ul_earfcn(self, band):
        
        binf = self.eutra_arr[int(band)]
        earfcn = 5*(binf[6] - binf[4]) + binf[7]
        return earfcn
    
def main():
    
    en = EutraEarfcn()
    
    while (True):
        
        print "1. DL freq to earfcn"
        print "2. UL freq to earfcn"
        print "3. DL earfcn to freq"
        print "4. UL earfcn to freq"
        print "q. Quit"
        
        op = raw_input("Select Calibration Option:")
        
        if (op == '1'):
            band = raw_input("band:")
            freq = raw_input("freq:")
            print str(en.dl_freq2earfcn(band, freq))
        elif (op == '2'):
            band = raw_input("band:")
            freq = raw_input("freq:")
            print str(en.ul_freq2earfcn(band, freq))
        elif (op == '3'):
            earfcn = raw_input("earfcn:")
            print str(en.dl_earfcn2freq(earfcn))
        elif (op == '4'):
            earfcn = raw_input("earfcn:")
            print str(en.ul_earfcn2freq(earfcn)) 
        elif (op == 'q'):
            break
        else:
            break
        
    print "bye bye"

if __name__ == "__main__":
    main()
    