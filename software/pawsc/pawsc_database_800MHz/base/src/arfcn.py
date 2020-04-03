from base.src import constants

def FreqToArfcn(band, tech, freq):
        
    if (tech=='LTE'):
        arfcn_table = constants.LTE_arfcn_table
        arfcn_dict = arfcn_table[band]


        # Check if given frequency is in downlink or uplink
        if (arfcn_dict['FDL_low'] > arfcn_dict['FUL_low']):
            if freq > arfcn_dict['FDL_low']:
                freq_DL = freq
                freq_UL = freq - arfcn_dict['spacing']
            else:
                freq_UL = freq
                freq_DL = freq + arfcn_dict['spacing']
        else:
            if freq > arfcn_dict['FUL_low']:
                freq_UL = freq
                freq_DL = freq + arfcn_dict['spacing']
            else:
                freq_DL = freq
                freq_UL = freq - arfcn_dict['spacing']

        arfcn_DL = int(arfcn_dict['NOffs-DL'] + 10*(freq_DL-arfcn_dict['FDL_low']))
        arfcn_UL = int(arfcn_dict['NOffs-UL'] + 10*(freq_UL- arfcn_dict['FUL_low']))

        return {'arfcn_DL': arfcn_DL, 'arfcn_UL': arfcn_UL}

    if (tech == 'GSM'):
        arfcn_table = constants.GSM_arfcn_table
        arfcn_dict = arfcn_table[band]

        if (arfcn_dict['FDL_low'] > arfcn_dict['FUL_low']):
            if freq > arfcn_dict['FDL_low']:
                freq_DL = freq
                freq_UL = freq - arfcn_dict['spacing']
            else:
                freq_UL = freq
                freq_DL = freq + arfcn_dict['spacing']

        freq_UL = freq_DL - arfcn_dict['spacing']
        arfcn = int(5*(freq_UL - 890))
        if (arfcn < 0): 
            arfcn = arfcn + 1024
        return {'arfcn_DL': arfcn, 'arfcn_UL':arfcn}



def FreqRangeToArfcnRange(band,tech,freq_start,freq_end,bw):

    if (freq_end-freq_start > bw):
        freq_start_A = freq_start+bw/2
        freq_end_A = freq_end-bw/2

        arfcn_start = FreqToArfcn(band,tech,freq_start_A)
        arfcn_end = FreqToArfcn(band,tech,freq_end_A)
        return {'arfcn_start': arfcn_start, 'arfcn_end': arfcn_end}
