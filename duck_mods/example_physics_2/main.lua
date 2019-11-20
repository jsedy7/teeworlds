-- main mod file
local laserLines = {}
local debugRects = {}

function makeLaserLine()
    return {
        x = 0,
        y = 0,
        w = 0,
        h = 0,
        solid = 0,
        hookable = 0,
    }
end


function arelaserLinesEqual(l1, l2)
    return l1.x == l2.x and l1.y == l2.y and l1.w == l2.w and l1.h == l2.h
end

function findLaserLineID(line)
    local i = 1
    for _,l in pairs(laserLines) do
        if arelaserLinesEqual(l, line) then
            return i
        end
        i = i + 1
    end
    return nil
end

function OnRender(LocalTime)
    TwRenderSetDrawSpace(1)

    for _,r in pairs(debugRects) do
        TwRenderSetColorU32(r.color)
        TwRenderQuad(r.x, r.y, r.w, r.h)
    end

    for _,l in pairs(laserLines) do
        if l.solid == 1 then
            local alpha = 0.7 + sin(LocalTime*3.0) * 0.1
            if l.hookable == 1 then
                TwRenderSetColorF4(0.84, 0.34, 1.0, alpha)
            else
                TwRenderSetColorF4(1.0, 0.84, 0.34, alpha)
            end
            TwRenderQuad(l.x, l.y, l.w, l.h)

            TwRenderSetColorF4(1.0, 1.0, 1.0, 0.5)
            TwRenderQuad(l.x, l.y, l.w, 2.0)
            TwRenderQuad(l.x, l.y + l.h - 2.0, l.w, 2.0)
        end
    end
end

function OnMessage(packet)
    if packet.mod_id == 0x1 then -- DEBUG_RECT
        local rect = TwNetPacketUnpack(packet, {
            "i32_id",
            "float_x",
            "float_y",
            "float_w",
            "float_h",
            "u32_color",
        })

        debugRects[rect.id] = rect
    end

    if packet.mod_id == 0x2 then -- MAP_RECT_SET_SOLID
        local rect = TwNetPacketUnpack(packet, {
            "i32_solid",
            "i32_hookable",
            "i32_tx",
            "i32_ty",
            "i32_tw",
            "i32_th",
        })

        local lline = makeLaserLine()
        lline.solid = rect.solid
        lline.hookable = rect.hookable
        lline.x = rect.tx * 32
        lline.y = rect.ty * 32
        lline.w = rect.tw * 32
        lline.h = rect.th * 32

        local lineID = findLaserLineID(lline)
        if lineID then
            laserLines[lineID] = lline
        else 
            laserLines[#laserLines+1] = lline
        end

        --printObj(lline)

        if rect.solid == 0 then
            for x = 0, rect.tw-1, 1 do
                for y = 0, rect.th-1, 1 do
                    TwPhysSetTileCollisionFlags(rect.tx + x, rect.ty + y, 0) -- air
                end
            end
        else
            for x = 0, rect.tw-1, 1 do
                for y = 0, rect.th-1, 1 do
                    local flags = 1
                    if rect.hookable then
                        flags = flags + 4
                    end
                    TwPhysSetTileCollisionFlags(rect.tx + x, rect.ty + y, flags) -- solid
                end
            end
        end
    end
end