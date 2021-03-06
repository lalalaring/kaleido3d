import os, shutil, zipfile

def copy_files_by_ext(file_dir, file_ext, target_dir):
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    for _file in os.listdir(file_dir):
        if _file.endswith(file_ext):
            shutil.copyfile(os.path.join(file_dir, _file), os.path.join(target_dir, _file))

def zipdir(path, ziph):
    for root, dirs, files in os.walk(path):
        for _file in files:
            ziph.write(os.path.join(root, _file), os.path.join(root[len(path)+1:], _file))