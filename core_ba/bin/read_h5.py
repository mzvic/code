import h5py

file_path = 'storage.h5'

with h5py.File(file_path, 'r') as file:
    if '/DATA' in file:
        dataset = file['/DATA']
        data = dataset[:]
        print(data)
        
