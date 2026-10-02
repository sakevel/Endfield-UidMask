local zml=loadstring(LuaManagerInst:LoadLua('ZML/Api'))()
typeof=function(x)return x end
NotNull=function(x)return x~=nil end
local counters={}
local function label(name,active,scene,throws)
    counters[name]=0
    return {text='raw-uid-1000123456',gameObject={activeInHierarchy=active,scene={IsValid=function()return scene end}},
        RefreshPopulateText=function() if throws then error('destroyed') end counters[name]=counters[name]+1 end,
        SetVerticesDirty=function()end,SetLayoutDirty=function()end}
end
local labels={Length=4,[0]=label('hud',true,true),[1]=label('card',true,true),[2]=label('hidden',false,true),[3]=label('asset',true,false)}
CS={UnityEngine={Resources={FindObjectsOfTypeAll=function()return labels end}},TMPro={TMP_Text={}}}
local ctrl={}
H.bind(ctrl);H.bind(ctrl) -- rebind must unsubscribe previous observer.
assert(zml.set('uid-mask','alias_uid','876543210'))
assert(counters.hud==1 and counters.card==1 and counters.hidden==0 and counters.asset==0)
assert(labels[0].text=='raw-uid-1000123456') -- Never read/write original text.
H.close(ctrl);assert(zml.set('uid-mask','enabled',false));assert(counters.hud==1)
H.bind(ctrl);labels[4]=label('destroyed',true,true,true);labels.Length=5
assert(zml.set('uid-mask','enabled',true));assert(counters.hud==2)
H.close(ctrl)
local inputCallback,buttonCallback,feedback,refreshes
local ctx={parent={},width=1200,get=function()return zml.get('uid-mask')end,set=function(k,v)return zml.set('uid-mask',k,v)end,
    text=function(_,s)feedback={text=s};return feedback end,
    button=function(_,_,_,_,_,fn)buttonCallback=fn end,
    input=function(_,_,_,_,_,_,fn)inputCallback=fn end,
    refresh=function()refreshes=(refreshes or 0)+1 end}
Entry.create(ctx)
for _,invalid in ipairs({'','abc','1e9','123\n4','<b>12','１２３','123456789012345678901'})do
    inputCallback(invalid);assert(zml.get('uid-mask').alias_uid=='876543210')
end
inputCallback('99999999999999999999');assert(zml.get('uid-mask').alias_uid=='99999999999999999999')
buttonCallback();assert(zml.get('uid-mask').enabled=='false' and refreshes==1)
assert(zml.config_entry('uid-mask').api==1)
function setupHL()
    HL={Any={},Boolean={},Table={},Thread={},Number={}}
    HL.Class=function()return setmetatable({},{__newindex=function(t,k,v)
        if type(v)~='table' or not v.unsetField then rawset(t,k,v) end
    end})end
    HL.Field=function()return setmetatable({unsetField=true},{__shl=function(_,v)return v end})end
    HL.StaticField=function()return setmetatable({},{__shl=function(_,v)return v end})end
    HL.Override=function()return setmetatable({},{__shl=function(_,v)return v end})end
    HL.Method=function()
        local value=setmetatable({},{__shl=function(_,v)return v end})
        value.Return=function()return value end
        return value
    end
    HL.StaticMethod=HL.Method
    HL.Commit=function()end
    PanelId={UIDPanel=1};BEYOND_INNER_DEBUG=false;UIConst={COMMON_UI_TIME_UPDATE_INTERVAL=1}
    CS.Beyond={CloudGame={enabled=true}}
    GameInstance={netClientManager={},player={playerInfoSystem={roleId='1000123456'}}}
    require_ex=function()return {UICtrl={}}end
    coroutine.wait=function()end
end
function verifyController()
    local v={text={text=''},pingCon={gameObject={SetActive=function()end}}}
    local c=setmetatable({view=v},{__index=UIDPanelCtrl})
    c:OnCreate(nil);assert(v.text.text=='UID: 1000123456')
    local before=counters.hud;assert(zml.set('uid-mask','enabled',true));assert(counters.hud==before+1)
    c:OnClose();before=counters.hud;assert(zml.set('uid-mask','enabled',false));assert(counters.hud==before)
end
