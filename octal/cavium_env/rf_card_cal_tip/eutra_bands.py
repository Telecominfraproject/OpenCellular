#!/usr/bin/env python

from types import TypeType

class Band(object):
    @staticmethod
    def containsBand(eutra_band):
        return 0
    def dl_freq(self):
        return (0, 0, 0)    # (low, middle, high)
    def ul_freq(self):
        return (0, 0, 0)
    
class Band_1(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band1", 1]
    def dl_freq(self):
        return (2110, 2140, 2170)
    def ul_freq(self):
        return (1920, 1950, 1980)
        
class Band_3(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band3", 3]
    def dl_freq(self):
        return (1805, 1842.5, 1880) # 1850, 1755
    def ul_freq(self):
        return (1710, 1747.5, 1785)
        
class Band_4(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band4", 4]
    def dl_freq(self):
        return (2110, 2132.5, 2155)
    def ul_freq(self):
        return (1710, 1732.5, 1755)
    
class Band_7(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band7", 7]
    def dl_freq(self):
        return (2620, 2655, 2690)
    def ul_freq(self):
        return (2500, 2535, 2570)
    
class Band_8(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band8", 8]
    def dl_freq(self):
        return (925, 942.5, 960)
    def ul_freq(self):
        return (880, 897.5, 915)
    
class Band_13(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band13", 13]
    def dl_freq(self):
        return (746, 751, 756)
    def ul_freq(self):
        return (777, 782, 787)
    
class Band_14(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band14", 14]
    def dl_freq(self):
        return (758, 763, 768)
    def ul_freq(self):
        return (788, 793, 798)
    
class Band_17(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band17", 17]
    def dl_freq(self):
        return (734, 740, 746)
    def ul_freq(self):
        return (704, 710, 716)

class Band_25(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band25", 25]
    def dl_freq(self):
        return (1930, 1962, 1995)
    def ul_freq(self):
        return (1850, 1882, 1915)

class Band_28(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band28", 28]
    def dl_freq(self):
        return (758, 780.5, 803)
    def ul_freq(self):
        return (703, 725.5, 748)

class Band_38(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band38", 38]
    def dl_freq(self):
        return (2570, 2595, 2620) #2583, 2609, 2595
    def ul_freq(self):
        return (2570, 2595, 2620)

class Band_40(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band40", 40]
    def dl_freq(self):
        return (2300, 2350, 2400)
    def ul_freq(self):
        return (2300, 2350, 2400)
        
class Band_41(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band41", 41]
    def dl_freq(self):
        return (2496, 2593, 2690)
    def ul_freq(self):
        return (2496, 2593, 2690)
        
class Band_42(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band42", 42]
    def dl_freq(self):
        return (3400, 3500, 3600)
    def ul_freq(self):
        return (3400, 3500, 3600)

class Band_43(Band):
    @staticmethod
    def containsBand(eutra_band):
        return eutra_band in ["band43", 43]
    def dl_freq(self):
        return (3600, 3650, 3800)
    def ul_freq(self):
        return (3600, 3650, 3800)
   
class BandFactory(object):
    @staticmethod
    def newBand(eutra_band):
        # walk through all Band classes
        bandClasses = [j for (i, j) in globals().iteritems() if isinstance(j, TypeType) and issubclass(j, Band)]
        for bandClass in bandClasses :
            if bandClass.containsBand(eutra_band):
                return bandClass()
        #if research was unsuccessful, raise an error
        raise ValueError('No E-UTRA band containing "%s".' % eutra_band)
    
def main():
    band = 4
    mb = BandFactory().newBand(band)
    dl_freq = mb.dl_freq()
    ul_freq = mb.ul_freq()
    print("Band=" + str(band) + " downlink=" + str(dl_freq[1]) + " uplink=" + str(ul_freq[1]))
    
if __name__ == "__main__":
    main()
    
