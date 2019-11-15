-- utils.lua

function TableToStr(t)
    out = "{ "

    for k,v in pairs(t) do
        if type(v) == "table" then
            out = out .. "[" .. k .. "] = " .. TableToStr(v) .. ", "
        elseif type(v) == "boolean" then
            s = "false"
            if v then s = "true" end
            out = out .. "[" .. k .. "] = " .. s .. ", "
        else
            out = out .. "[" .. k .. "] = " .. v .. ", "
        end
    end

    out = out .. "}"
    return out
end

function PrintTable(t, name)
    pre = ""
    if name then
        pre = name .. " = "
    end

    print(pre .. TableToStr(t))
end

function vec2(x_, y_)
    return { x= x_, y= y_ }
end

function vec2Length(v)
    return sqrt(v.x*v.x + v.y*v.y)
end

function vec2Normalize(v)
    local f = 1.0 / vec2Length(v)
    return vec2(v.x * f, v.y * f)
end

function vec2MulScalar(v, s)
    return vec2(v.x * s, v.y * s)
end

function vec2Sub(v1, v2)
    return vec2(v1.x - v2.x, v1.y - v2.y)
end

function vec2Angle(v)
    return atan2(v.y, v.x)
end

function vec2Rotate(v, angle)
    local c = cos(angle)
    local s = sin(angle)
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y)
end


function CalcScreenParams(Amount, WMax, HMax, Aspect)
    local f = sqrt(Amount) / sqrt(Aspect)
    local w = f*Aspect
    local h = f

    -- limit the view
    if w > WMax then
        w = WMax
        h = w/Aspect
    end

    if h > HMax then
        h = HMax
        w = h*Aspect
    end
    
    return {w, h}
end

function GetWorldViewRect(CenterX, CenterY, Aspect, Zoom)
    local params = CalcScreenParams(1150*1000, 1500, 1050, Aspect)
    local Width = params[1]
    local Height = params[2]
    Width = Width * Zoom
    Height = Height * Zoom
    
    return {
        x = CenterX-Width/2,
        y = CenterY-Height/2,
        w = Width,
        h = Height,
    }
end

function IsPointInsideRect(point, rect)
    return (point.x >= rect.x and point.x < rect.x + rect.w and point.y >= rect.y and point.y < rect.y + rect.h)
end

function DrawRect(rect, rectColor)
    TwRenderSetTexture(-1)
    TwRenderSetColorF4(rectColor[1], rectColor[2], rectColor[3], rectColor[4])
    TwRenderQuad(rect.x, rect.y, rect.w, rect.h)
end

function DrawBorderRect(rect, borderWith, rectColor, borderColor)
    local pixelScale = TwGetPixelScale()
    local borderW = borderWith * pixelScale.x
    local borderH = borderWith * pixelScale.y
    
    TwRenderSetTexture(-1)

    TwRenderSetColorF4(rectColor[1], rectColor[2], rectColor[3], rectColor[4])
    TwRenderQuad(rect.x, rect.y, rect.w, rect.h)

    TwRenderSetColorF4(borderColor[1], borderColor[2], borderColor[3], borderColor[4])
    TwRenderQuad(rect.x, rect.y, rect.w, borderH)
    TwRenderQuad(rect.x, rect.y + rect.h - borderH, rect.w, borderH)
    TwRenderQuad(rect.x, rect.y, borderW, rect.h)
    TwRenderQuad(rect.x + rect.w - borderW, rect.y, borderW, rect.h)
end

function clamp(val, min, max)
    if val < min then
        return min
    elseif val > max then
        return max
    end
    return val
end

function max(val, max)
    if val > max then
        return max
    end
    return val
end

function distance(v1, v2)
    return vec2Length(vec2Sub(v1, v2))
end