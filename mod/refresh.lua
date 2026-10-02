-- Trigger UI redraw on configuration updates.
local H, states = {}, setmetatable({}, {__mode="k"})
local api
local function service()
    if not api then
        local f, err = loadstring(LuaManagerInst:LoadLua("ZML/Api"), "@ZML/Api")
        assert(f, err); api = f(); assert(api.api_version == 1)
    end
    return api
end
local function refresh()
    local labels = CS.UnityEngine.Resources.FindObjectsOfTypeAll(typeof(CS.TMPro.TMP_Text))
    assert(labels.Length <= 20000)
    local count, errors=0,0
    for i=0,labels.Length-1 do
        local label=labels[i]
        if label and NotNull(label) then
            local ok=pcall(function()
                local go=label.gameObject
                -- Redraw complete
                if go.activeInHierarchy and go.scene:IsValid() then
                    label:RefreshPopulateText()
                    label:SetVerticesDirty()
                    label:SetLayoutDirty()
                    count=count+1
                end
            end)
            if not ok then errors=errors+1 end
        end
    end
    return count, errors
end
function H.close(ctrl)
    local cancel=states[ctrl]; states[ctrl]=nil
    if cancel then pcall(cancel) end
end
function H.bind(ctrl)
    H.close(ctrl)
    local ok=pcall(function()
        local zml=service()
        states[ctrl]=zml.subscribe("uid-mask",function()
            local ok,count,errors=pcall(refresh)
            local event=not ok and "refresh_error" or errors>0 and "refresh_partial" or
                count==0 and "refresh_empty" or "refresh_requested"
            zml.report("uid-mask",event)
        end)
        zml.report("uid-mask","refresh_bound")
    end)
    if not ok and api then pcall(api.report,"uid-mask","refresh_error") end
end
return H
