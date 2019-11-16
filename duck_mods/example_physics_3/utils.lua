-- utils.lua

function TableToStr(t)
    out = "{ "

    for k,v in pairs(t) do
        if type(v) == "table" then
            out = out .. "[" .. k .. "] = " .. TableToStr(v) .. ", "
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
