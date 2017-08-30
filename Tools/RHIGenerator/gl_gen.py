from glad.spec import GLSpec,SPECS
from glad.parse import Spec
from glad.generate import Generator


spec = SPECS['gl']
spec.profile = 'core'
spec.from_svn()

gen = Generator('openg.l', spec, ['gl', 'gles2'])
gen.generate()