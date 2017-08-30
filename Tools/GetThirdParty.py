from CI.Download import download_and_extract
import argparse, sys

parser = argparse.ArgumentParser(description='Parse Dependency Options')

if len(sys.argv) < 2:
    print 'options needed'
    sys.exit(1)

parser.add_argument('--output_dir', help='specify output file path')
parser.add_argument('--target_os', help='available os are \'Windows\',\'WindowsUWP\',\'Android\',\'MacOS\'')

args = parser.parse_args(sys.argv[1:])

libs = [
    {
        'name'  : 'third_party_c',
        'win64' : 
        {
            'url'   : 'https://ci.appveyor.com/api/projects/tomicyo/third-party-clibs/artifacts/build/third_party_clibs_windows_shared.zip'
        }
    },
    {
        'name'  : 'third_party',
        'win64' : 
        {
            'url'   : 'https://ci.appveyor.com/api/projects/tomicyo/third-party/artifacts/artifacts/third_party_windows.zip'    
        }
    },
    {
        'name'  : 'protobuf',
        'win64' : 
        {
            'url'   : 'https://ci.appveyor.com/api/buildjobs/ic77nh5lpobvkmxw/artifacts/output%2Fprotobuf_md_windows.zip'
        }
    },
    {
        'name'  : 'grpc',
        'win64' : 
        {
            'url'   : 'https://ci.appveyor.com/api/projects/tomicyo/grpc/artifacts/output/grpc_windows.zip'
        }
    }
]

for lib in libs:
    download_and_extract(lib['win64']['url'], args.output_dir)

