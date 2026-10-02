"""Own Lua/actual public loader API; no Unity/game/data dependencies."""
import pathlib,sys,subprocess
sys.path.insert(0,sys.argv[1])
from lupa.lua54 import LuaRuntime
root=pathlib.Path(__file__).resolve().parents[1]
server=subprocess.Popen([sys.argv[2],'--serve',str(root/'mod/mod.ini')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
def route(_,path):
    server.stdin.write((path+'\n').encode());server.stdin.flush()
    n=int(server.stdout.readline());s=server.stdout.read(n);assert len(s)==n
    return s.decode('utf8')
try:
    lua=LuaRuntime(unpack_returned_tuples=True);lua.globals().route=route
    lua.execute('loadstring=load; LuaManagerInst={LoadLua=function(self,p)return route(self,p)end}')
    lua.execute(route(None,'ZML/Api'))
    lua.globals().H=lua.execute((root/'mod/refresh.lua').read_text(encoding='utf8'))
    lua.execute((root/'tests/ui_mock.lua').read_text(encoding='utf8'))
    if len(sys.argv)==4:
        lua.execute('setupHL()')
        lua.execute(pathlib.Path(sys.argv[3]).read_text(encoding='utf8'))
        lua.execute('verifyController()')
    print('PASS: real loader standard config/subscription, string validation, native-redraw lifecycle'+(', actual patched current controller' if len(sys.argv)==4 else ''))
finally:
    server.stdin.close()
    try:server.wait(timeout=5)
    except subprocess.TimeoutExpired:server.kill();server.wait()
    if server.returncode:raise RuntimeError(server.stderr.read().decode(errors='replace'))
