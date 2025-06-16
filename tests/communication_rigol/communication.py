import pyvisa

rm = pyvisa.ResourceManager('@py')
inst = rm.open_resource('USB0::6833::1301::MS5A242205632::0::INSTR',
                        write_termination='\n', read_termination='\n')

command = input("Enter command: ")

if command.endswith('?'):
    response = inst.query(command)
    print(response.strip())
else:
    inst.write(command)

