return {api=1,create=function(ctx)
    local values=assert(ctx.get())
    ctx.text(ctx.parent,"仅伪装本地画面；真实 UID、游戏业务与复制结果不变。",24,20,ctx.width-48,52,25)
    ctx.button(ctx.parent,values.enabled=="true" and "UID 伪装：开启（点击关闭）" or "UID 伪装：关闭（点击开启）",24,88,480,function()
        local v=assert(ctx.get());local ok=ctx.set("enabled",v.enabled~="true")
        if ok then ctx.refresh() end
    end)
    ctx.text(ctx.parent,"显示的 UID（1–20 位纯数字）",24,162,ctx.width-48,48,25)
    local feedback=ctx.text(ctx.parent,"修改后立即刷新已打开的文字。UID 不是账号身份。",24,292,ctx.width-48,70,23)
    ctx.input(ctx.parent,values.alias_uid or "1000000000","例如 1000000000",24,218,600,function(value)
        if type(value)~="string" or #value<1 or #value>20 or not value:match("^%d+$") then
            feedback.text="未保存：请输入 1–20 位纯数字。";return
        end
        local ok=ctx.set("alias_uid",value)
        feedback.text=ok and "已保存并请求刷新画面。" or "保存失败，原配置保持不变。"
    end,20)
end}
