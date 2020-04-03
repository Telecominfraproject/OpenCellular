def FreqToArfcn(band, tech, freq):

    LTE_arfcn_table = {
        'Band1':{'FDL_low':2110, 'NOffs-DL':0, 'FUL_low': 1920, 'NOffs-UL':18000, 'spacing': 190},
        'Band2':{'FDL_low':2110, 'NOffs-DL':0, 'FUL_low': 1920, 'NOffs-UL':18000, 'spacing': 80},
        'Band3':{'FDL_low':2110, 'NOffs-DL':0, 'FUL_low': 1920, 'NOffs-UL':18000, 'spacing': 95},
        'Band20':{'FDL_low':791, 'NOffs-DL':6150, 'FUL_low': 832, 'NOffs-UL':24150, 'spacing': -41 }
    }

    GSM_arfcn_table = {
        '900E':{'FDL_low': 925.2, 'FDL_high': 959.8, 'FUL_low': 880.2, 'FUL_high': 914.8, 'spacing': 45},
        '900R':{'FDL_low': 921.2, 'FDL_high': 959.8, 'FUL_low': 876.2, 'FUL_high': 914.8, 'spacing': 45},
        '900P':{'FDL_low': 935.2, 'FDL_high': 959.8, 'FUL_low': 890.2, 'FUL_high': 914.8, 'spacing': 45}
    }

        
    if (tech=='LTE'):
        arfcn_table = LTE_arfcn_table
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
        arfcn_table = GSM_arfcn_table
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
