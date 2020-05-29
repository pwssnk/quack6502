#!/usr/bin/env python

#### logcmp.py #####################################################
# This is a simple tool to aid in debugging the implementation     #
# of the MOS6502 CPU. The CPU debugging code will generate a       #
# JSON-formatted log file containing information about the         #
# CPU state before and after every cycle. These logs easily        #
# end up having tens of thousands of entries, so manually checking #
# them is a nightmare. This dirty Python script allows you to      #
# compare the CPU log file to a log file from a known to be        #
# correct implementation by an established emulator. It will point #
# out where the CPU states start to diverge, to give a starting    #
# place for debugging the CPU code.                                #
####################################################################

import sys
import json


def min(a, b):
    if a > b:
        return b
    else:
        return a


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: python logcmp.py [log a] [log b] [--flags]')
        print('Use flag --all to keep going after first mismatch')
        print('Use flag --ignore-unimplemented to ignore op mismatch for unimplemented opcodes (XXX\'s)')
        sys.exit(0)

    with open(sys.argv[1], 'r') as f:
        log_a = json.loads(f.read())

    with open(sys.argv[2], 'r') as f:
        log_b = json.loads(f.read())

    print('Log A contains {} entries\nLog B contains {} entries\n'.format(len(log_a), len(log_b)))

    check_cycles = True
    check_opcode = True
    check_registers = True
    
    stop_after_first_error = True
    ignore_xxx = False
    xxx_ignored = 0
    
    no_errors = True
    
    if '--all' in sys.argv:
        stop_after_first_error = False
        
    if '--ignore-unimplemented' in sys.argv:
        ignore_xxx = True

    for i in range(0, min(len(log_a), len(log_b))):
        if stop_after_first_error and not no_errors:
            break

        a = log_a[i]
        b = log_b[i]

        # Opcode and mnemonic
        if check_opcode:
            if int(a['Opcode'], 16) != int(b['Opcode'], 16) or a['OpcodeMnemonic'] != b['OpcodeMnemonic']:
                if ignore_xxx and (a['OpcodeMnemonic'] == 'XXX' or b['OpcodeMnemonic'] == 'XXX'):
                    xxx_ignored += 1
                else:
                    print('Opcode mismatch at cycle {} ({})'.format(a['Cycle'], b['Cycle']))
                    print('\tLog A:\t{} [{}]\n\tLog B:\t{} [{}]'.format(a['OpcodeMnemonic'], a['Opcode'].upper(),
                                                                        b['OpcodeMnemonic'], b['Opcode'].upper()))
                    print()
                    no_errors = False

        if check_cycles:
            if int(a['Cycle']) != int(b['Cycle']):
                print('CPU cycle count mismatch at cycle {} ({})'.format(a['Cycle'], b['Cycle']))
                print('\tLog A:\t{}\n\tLog B:\t{}'.format(a['Cycle'], b['Cycle']))
                print('\n\t Op: {} ({})\n'.format(a['OpcodeMnemonic'], b['OpcodeMnemonic']))
                no_errors = False

        if check_registers:
            regs_a = a['PreOpState']
            regs_b = b['PreOpState']

            keys = regs_a.keys()

            for key in keys:
                if int(regs_a[key], 16) != int(regs_b[key], 16):
                    print('Register mismatch at cycle {} ({})'.format(a['Cycle'], b['Cycle']))
                    print('\t  Reg\tLog A\tLog B')
                    print('\t  ---\t-----\t-----')
                    for k in keys:
                        if int(regs_a[k], 16) != int(regs_b[k], 16):
                            print('\t> {}\t{}\t{}'.format(k, regs_a[k].upper(), regs_b[k].upper()))
                        else:
                            print('\t  {}\t{}\t{}'.format(k, regs_a[k].upper(), regs_b[k].upper()))
                    print('\n\t Op: {} ({})\n'.format(a['OpcodeMnemonic'], b['OpcodeMnemonic']))
                    no_errors = False



    if no_errors:
        if ignore_xxx:
            print('No mismatch found (ignored {} unimplemented opcodes)'.format(xxx_ignored))
        else:
            print('No mismatch found')

