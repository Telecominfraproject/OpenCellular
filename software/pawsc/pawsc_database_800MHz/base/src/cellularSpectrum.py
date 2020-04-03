import re, json
import requests
from io import BytesIO
import sys

from base.src import constants
from base.src.arfcn import FreqRangeToArfcnRange
from base.src.calculation_engine import getSpectrumGLSD

def  convertCellular(channelWidths, spectrumRES, technology, band):
    # Iterate through spectra
    

    # Constrain spectra to technology / band
    if (technology == 'LTE'):
        arfcn_table = constants.LTE_arfcn_table
        if (band == 'Band20' or band == 'Band1'):
            FDL_low = int(arfcn_table[band]['FDL_low'])*1000000
            FDL_high = int(arfcn_table[band]['FDL_high'])*1000000
            FUL_low = int(arfcn_table[band]['FUL_low'])*1000000
            FUL_high = int(arfcn_table[band]['FUL_high'])*1000000
            spacing = FDL_low - FUL_low
    
    cell_ws_DL = []
    cell_ws_UL = []

    # Assume we have an ordered list - extract cellular white space blocks
    for schedules in spectrumRES:
        for spectra in schedules['spectrumSchedules']:
            for profile in spectra['spectra']:
                # All spectra is listed in start stop pairs and is ordered
                for specEntry in profile['profiles']:
                    startHz = specEntry[0]['hz']
                    startDB = specEntry[0]['dbm']
                    endHz = specEntry[1]['hz']
                    endDB = specEntry[1]['dbm']
                  
                    # Case of White space fully inside or right of cellular downlink
                    if startHz >= FDL_low and startHz <= FDL_high:
                        cell_ws_DL_start = startHz
                        if endHz <= FDL_high:
                            cell_ws_DL_end = endHz
                        else:
                            cell_ws_DL_end = FDL_high
                        # Add start and end cellular white space block to DL list
                        cell_ws_DL.append([cell_ws_DL_start,startDB,cell_ws_DL_end,startDB])

                    # Case of White Space containing or to left of cellular downlink
                    if startHz < FDL_low and endHz > FDL_low:
                        cell_ws_DL_start = FDL_low
                        if endHz <= FDL_high:
                            cell_ws_DL_end = endHz
                        else:
                            cell_ws_DL_end = FDL_high
                        cell_ws_DL.append([cell_ws_DL_start,startDB,cell_ws_DL_end,startDB])

                     # Case of White space fully inside or right of cellular downlink
                    if startHz >= FUL_low and startHz <= FUL_high:
                        cell_ws_UL_start = startHz
                        if endHz <= FUL_high:
                            cell_ws_UL_end = endHz
                        else:
                            cell_ws_UL_end = FUL_high
                        # Add start and end cellular white space block to DL list
                        cell_ws_UL.append([cell_ws_UL_start,startDB,cell_ws_UL_end,startDB])

                    # Case of White Space containing or to left of cellular uplink
                    if startHz < FUL_low and endHz > FUL_low:
                        cell_ws_UL_start = FUL_low
                        if endHz <= FUL_high:
                            cell_ws_UL_end = endHz
                        else:
                            cell_ws_UL_end = FUL_high
                        cell_ws_UL.append([cell_ws_UL_start,startDB,cell_ws_UL_end,startDB])



    
    # Align cellular white space to FDD if tech is FDD
    #print ('Cellular white space with no FDD')
    #print ('DL :', cell_ws_DL)
    #print ('UL :', cell_ws_UL)
    
    # Find downlink shadow block for uplink block
    cell_ws_UL_shadow = []
    for ul_blocks in cell_ws_UL:
        shadow_low = ul_blocks[0] + spacing
        shadow_high = ul_blocks[2] + spacing
        cell_ws_UL_shadow.append([shadow_low,shadow_high])

    # Find uplink shadow block for downlink block
    cell_ws_DL_shadow = []
    for dl_blocks in cell_ws_DL:
        shadow_low = dl_blocks[0] - spacing
        shadow_high = dl_blocks[2] - spacing
        cell_ws_DL_shadow.append([shadow_low,shadow_high])

    #print ('Cellular white space shadow spectrum')
    #print ('DL shadow :', cell_ws_DL_shadow)
    #print ('UL shadow :', cell_ws_UL_shadow)



    cell_ws_DL_final = []
    cell_ws_UL_final = []  

    # Restrict downlink block to only available in uplink shadow
    for UL_shadow_pair in cell_ws_UL_shadow:
        UL_shadow_start = UL_shadow_pair[0]
        UL_shadow_end = UL_shadow_pair[1]

        for ws_DL_pair in cell_ws_DL:
            DL_start = ws_DL_pair[0]
            DL_start_db = ws_DL_pair[1]
            DL_end = ws_DL_pair[2]
            DL_end_db = ws_DL_pair[3]

  
            if DL_start >= UL_shadow_start and DL_end <= UL_shadow_end: 
                startHz = DL_start
                endHz = DL_end
                cell_ws_DL_final.append([startHz,DL_start_db,endHz,DL_end_db])
            elif UL_shadow_start > DL_start and UL_shadow_end < DL_end: 
                startHz = UL_shadow_start
                endHz = UL_shadow_end
                cell_ws_DL_final.append([startHz,DL_start_db,endHz,DL_end_db])
            elif DL_start <= UL_shadow_start and DL_end > UL_shadow_start and DL_end <= UL_shadow_end:
                startHz = UL_shadow_start
                endHz = DL_end
                cell_ws_DL_final.append([startHz,DL_start_db,endHz,DL_end_db])
            elif UL_shadow_start <= DL_start and UL_shadow_end > DL_start and UL_shadow_end <= DL_end:
                startHz = DL_start
                endHz = UL_shadow_end
                cell_ws_DL_final.append([startHz,DL_start_db,endHz,DL_end_db])

    # Restrict uplink block to only available in downlink shadow
    for DL_shadow_pair in cell_ws_DL_shadow:
        DL_shadow_start = DL_shadow_pair[0]
        DL_shadow_end = DL_shadow_pair[1]

        #print (DL_shadow_start, DL_shadow_end)
        for ws_UL_pair in cell_ws_UL:
            UL_start = ws_UL_pair[0]
            UL_start_db = ws_UL_pair[1]
            UL_end = ws_UL_pair[2]
            UL_end_db = ws_UL_pair[3]

            #print ('UL start, end: ',UL_start, UL_end)

            if UL_start >= DL_shadow_start and UL_end <= DL_shadow_end: 
                startHz = UL_start
                endHz = UL_end
                cell_ws_UL_final.append([startHz,UL_start_db,endHz,UL_end_db])
            elif DL_shadow_start > UL_start and DL_shadow_end < UL_end: 
                startHz = DL_shadow_start
                endHz = DL_shadow_end
                cell_ws_UL_final.append([startHz,UL_start_db,endHz,UL_end_db])
            elif UL_start <= DL_shadow_start and UL_end > DL_shadow_start and UL_end <= DL_shadow_end:
                startHz = DL_shadow_start
                endHz = UL_end
                cell_ws_UL_final.append([startHz,UL_start_db,endHz,UL_end_db])
            elif DL_shadow_start <= UL_start and DL_shadow_end > UL_start and DL_shadow_end <= UL_end:
                startHz = UL_start
                endHz = DL_shadow_end
                cell_ws_UL_final.append([startHz,UL_start_db,endHz,UL_end_db])
    
    
    #print ('Final Cellular white space ')
    #print ('DL :', cell_ws_DL_final)
    #print ('UL :', cell_ws_UL_final)

    # Request arfcn cellular blocks
    

    # Build up a spectra section of AVAIL_SPECTRUM_RESP
    spectra = []
   
    for cwProfile in channelWidths:
        chwidth = int(cwProfile/1000000)
        spectra_entry = {}
        profilesHz = []
        profilesN = []
        for DBlock, UBlock in zip(cell_ws_DL_final, cell_ws_UL_final):
            #print (DBlock,UBlock)
            Dfreq_start = round(DBlock[0]/100000000,3)
            Dfreq_start_pawsc = str(Dfreq_start) + 'e8'

            Dpower_start = round(DBlock[1],1)

            Dfreq_end = round(DBlock[2]/100000000,3)
            Dfreq_end_pawsc = str(Dfreq_end) + 'e8'

            Dpower_end = round(DBlock[3],1)

            Ufreq_start = round(UBlock[0]/100000000,3)
            Ufreq_start_pawsc = str(Ufreq_start) + 'e8'
          

            Upower_start = round(UBlock[1],1)

            Ufreq_end = round(UBlock[2]/100000000,3)
            Ufreq_end_pawsc = str(Ufreq_end) + 'e8'

            Upower_end = round(UBlock[3],1)

            # Check if channel width fits in available spectrum 
            if (Dfreq_end-Dfreq_start)*100 > chwidth:
                if not('resolutionBwHz' in spectra_entry):
                    chwidth_pawsc = str(chwidth) + 'e6'
                    spectra_entry['resolutionBwHz'] = chwidth_pawsc
                profilePair = []
                profileEntry = {}
            
                profileEntry['Dhz'] =  Dfreq_start_pawsc
                profileEntry['Uhz'] =  Ufreq_start_pawsc
                profileEntry['Ddbm'] = Dpower_start
                profileEntry['Udbm'] = Upower_start
                profilePair.append(profileEntry)
               

                profileEntry = {}
                profileEntry['Dhz'] =  Dfreq_end_pawsc
                profileEntry['Uhz'] =  Ufreq_end_pawsc
                profileEntry['Ddbm'] = Dpower_end
                profileEntry['Udbm'] = Upower_end
                profilePair.append(profileEntry)
               

                profilesHz.append(profilePair)

                # Get the arfcn values
                res  = FreqRangeToArfcnRange(band,technology,Dfreq_start*100,Dfreq_end*100,chwidth)
                profilePair = []
                profileEntry = {}
                profileEntry['DARFCN'] = res['arfcn_start']['arfcn_DL']
                profileEntry['UARFCN'] = res['arfcn_start']['arfcn_UL']
                profileEntry['Ddbm'] = Dpower_start
                profileEntry['Udbm'] = Upower_start
                profilePair.append(profileEntry)

                profileEntry = {}
                profileEntry['DARFCN'] = res['arfcn_end']['arfcn_DL']
                profileEntry['UARFCN'] = res['arfcn_end']['arfcn_UL']
                profileEntry['Ddbm'] = Dpower_end
                profileEntry['Udbm'] = Upower_end
                profilePair.append(profileEntry)

                profilesN.append(profilePair)

            if len(profilesHz) > 0:
                spectra_entry['profilesHz'] = profilesHz
                spectra_entry['profilesN'] = profilesN

        if spectra_entry:
            spectra.append(spectra_entry)

    return spectra




    '''
    # Downlink
    for cell_block in cell_ws_DL_final:
        freq_start = int(cell_block[0]/1000000)
        freq_end = int(cell_block[2]/1000000)
        for freqProfile in freqRanges:
            chwidth = int(freqProfile['channelWidthHz']/1000000)
           
            res  = FreqRangeToArfcnRange(band,technology,freq_start,freq_end,chwidth)
            print ('ARFCN DL result: ', res)
                
       
    # Uplink
    for cell_block in cell_ws_UL_final:
        freq_start = int(cell_block[0]/1000000)
        freq_end = int(cell_block[2]/1000000)
        for freqProfile in freqRanges:
            chwidth = int(freqProfile['channelWidthHz']/1000000)
        
            res  = FreqRangeToArfcnRange(band,technology,freq_start,freq_end,chwidth)
            print ('ARFCN UL result: ', res)
    '''