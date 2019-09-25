# build content
import os
import shutil
import glob
from subprocess import call

script_dir = os.path.dirname(os.path.realpath(__file__))
build_dir = os.path.join(script_dir, 'build_genie')
generated_dir = os.path.join(build_dir, 'src', 'generated')

# copy data files
datasrc = glob.glob(os.path.join(script_dir, 'datasrc') + '/**', recursive=True)
copyList = []

for f in datasrc:
    if f.endswith(('.png', '.wv', '.ttf', '.txt', '.map', '.rules', '.json', '.js')):
        copyList.append(f)

# make 'data' directory
data_dir = os.path.join(build_dir, 'data')
if not os.path.exists(data_dir):
    os.mkdir(data_dir)

for f in copyList:
    pathsplit = os.path.split(f)
    src_file = pathsplit[1]
    pathsplit = os.path.split(pathsplit[0])
    d = os.path.dirname(f)
    f_dirs = d[d.find('datasrc')+8:]
    dest_dir = os.path.join(build_dir, 'data', f_dirs)

    # make sub directories
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)

    # copy file
    shutil.copy2(f, os.path.join(dest_dir, src_file))
    print(dest_dir, src_file)

print('Content copied.')

# generate source files
# make 'src/generated' directory
if not os.path.exists(generated_dir):
    os.makedirs(generated_dir)

def generate_source(action, output):
    print('******* ' + action + ' : ' + os.path.join(generated_dir, output))
    os.system('python datasrc/compile.py ' + action + ' > ' + os.path.join(generated_dir, output))

generate_source('network_source', 'protocol.cpp')
generate_source('network_header', 'protocol.h')
generate_source('server_content_source', 'server_data.cpp')
generate_source('server_content_header', 'server_data.h')
generate_source('client_content_source', 'client_data.cpp')
generate_source('client_content_header', 'client_data.h')

# DUCK
print('******* netobj_js : ' + os.path.join(generated_dir, 'netobj_js.cpp'))
os.system('python datasrc/netobj_js.py > ' + os.path.join(generated_dir, 'netobj_js.cpp'))

def c_hash(list, output):
    os.system('python scripts/cmd5.py ' + ' '.join(list) + ' > ' + os.path.join(generated_dir, output))

c_hash(['src/engine/shared/protocol.h', 'src/game/tuning.h', 'src/game/gamecore.cpp', os.path.join(generated_dir, 'protocol.h')], 'nethash.cpp')

print('Done.')