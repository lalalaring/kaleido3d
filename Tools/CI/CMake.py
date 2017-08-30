from CI import *
import platform, subprocess, re, os

coustom_bld_toolchain_os = ['ios', 'android']

generators = {
    'windows'   : 'Visual Studio 15 2017 Win64',
    'windowsuwp': 'Visual Studio 15 2017 Win64',
    'linux'     : 'Unix Makefiles',
    'macos'     : 'Xcode',
    'ios'       : 'Xcode',
    'android'   : 'Android Gradle - Ninja'
}

def find_qt():
    re_qt_dir = re.compile(r'Using\sQt\sversion\s(\d+\.\d+\.\d+)\sin\s(.*)', re.I)
    qt_dir = None
    qm = subprocess.Popen(['qmake', '-v'], shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while qm.poll() is None:  
        line = qm.stdout.readline()  
        line = line.strip()
        info = re_qt_dir.findall(line)
        if len(info) > 0:  
            (qt_ver, qt_dir) = info[0]
            print('find qt {0} in {1}'.format(qt_ver, qt_dir))
    return qt_dir


def generate_cmake(config, src_dir, bld_dir, inst_dir, target_os):
    if target_os == 'android':
        from ci.android import build_android_instance
        return

    qt_dir = find_qt()

    config_mapping = {
        'debug': 'Debug',
        'release': 'Release'
    }

    build_cmd = ['cmake', 
        '-G' + generators[target_os],
        '-H{0}'.format(src_dir),
        '-B{0}'.format(bld_dir),
        '-DCMAKE_BUILD_TYPE={0}'.format(config_mapping[config]),
        '-DCMAKE_INSTALL_PREFIX={0}'.format(inst_dir)
        ]
    if qt_dir:
        build_cmd.append('-DQt5_DIR={0}'.format(os.path.join(qt_dir, 'cmake/Qt5')))

    P = subprocess.Popen(build_cmd)
    P.wait()
    
    if P.returncode != 0:
        print('Generate Failed!')
        return

def build_cmake(config, bld_dir, need_install = False):
    config_mapping = {
        'debug': 'Debug',
        'release': 'Release'
    }

    cmd_list = ['cmake', '--build', bld_dir, '--config', config_mapping[config]]
    if need_install:
        cmd_list.append('--target')
        cmd_list.append('install')

    P = subprocess.Popen(cmd_list)
    P.wait()