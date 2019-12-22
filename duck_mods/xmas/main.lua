-- main mod file
require("base.lua")
require("draw.lua")

local debug = false
local localtime = false
local explosions = {}
local presents = {}

function OnLoad()
    print("Xmas");
    TwSetHudPartsShown({
        health = false,
        armor = false,
        ammo = false,
        time = false,
        killfeed = false,
        score = false,
        chat = true,
        scoreboard = true,
        weapon_cursor = false
    })
end

function DebugDraw()
    local cores = TwPhysGetCores()
    local joints = TwPhysGetJoints()

    TwRenderSetDrawSpace(1)
    TwRenderSetTexture(-1)
    
    for k,core in pairs(cores) do
        TwRenderSetColorF4(1, 1, 1, 1)
        TwRenderDrawCircle(core.x, core.y, core.radius)

        TwRenderSetColorF4(1, 0, 1, 0.5)
        TwRenderQuadCentered(core.x, core.y, core.radius * 2, core.radius * 2)
    end

    for k,joint in pairs(joints) do
        if joint.core1_id ~= nil or joint.core2_id ~= nil then
            local core1 = cores[joint.core1_id]
            local core2 = cores[joint.core2_id]

            TwRenderSetColorF4(0, 0.2, 1, 0.85)
            TwRenderDrawLine(core1.x, core1.y, core2.x, core2.y, 10)
        end
    end
end

function ExplosionSizeFunction(t)
    if t < 0.2 then
        return t/0.2
    end
    return cos((t-0.2)*1/0.8)
end

function OnRender(LocalTime, intraTick)
    localtime = LocalTime

    TwRenderSetDrawSpace(1)

    local cores = TwPhysGetCores()
    local joints = TwPhysGetJoints()

    local santa = cores[2]
    if santa then
        DrawSantee(santa.x, santa.y)

        -- rein deers
        local reindeerPos = {}
        for r = 0,3,1 do
            reindeerPos[r+1] = {
                x = santa.x - 350 - 120*r,
                y = santa.y + 40 + sin((LocalTime + r) * 3.1) * 10,
            }
        end

        -- reins
        local firstRd = reindeerPos[1]
        TwRenderSetTexture(-1)
        TwRenderSetColorF4(32/255, 12/255, 0, 1)
        TwRenderDrawLine(
            santa.x - 205,
            santa.y + 36,
            firstRd.x,
            firstRd.y,
            10
        )

        for r = 1,3,1 do
            local rd1 = reindeerPos[r]
            local rd2 = reindeerPos[r+1]
            TwRenderDrawLine(
                rd1.x,
                rd1.y,
                rd2.x,
                rd2.y,
                10
            )
        end
        
        -- deers
        for r = 1,4,1 do
            local rd1 = reindeerPos[r]
            DrawReindeer(rd1.x, rd1.y, LocalTime)
        end

        -- bag
        local bagCore = cores[1]
        if bagCore then
            TwRenderSetTexture(TwGetModTexture("red_bag"))
            TwRenderSetColorF4(1, 1, 1, 1)
            TwRenderSetQuadSubSet(0, 0, 1, 1)
            TwRenderQuadCentered(bagCore.x, bagCore.y-20, 150, 150)
        end

        -- sledge
        TwRenderSetTexture(TwGetModTexture("sledge"))
        TwRenderSetColorF4(1, 1, 1, 1)
        TwRenderSetQuadSubSet(0, 0, 1, 1)
        TwRenderQuadCentered(santa.x+6, santa.y+36, 600, 600/4)
    end

    -- do explosions
    for k,exp in ipairs(explosions) do
        local size = exp.radius*2 * ExplosionSizeFunction((LocalTime - exp.tstart) * 5)
        TwRenderSetTexture(TwGetBaseTexture(Teeworlds.IMAGE_PARTICLES))
        TwRenderSetColorU32(exp.color)
        TwRenderSetQuadRotation(exp.angle)
        SelectSprite({ cx = 8, cy = 8 }, 4, 1, 2, 2)
        TwRenderQuadCentered(exp.x, exp.y, size, size)

        if size < 0 then
            explosions[k] = nil
        end
    end

    TwRenderSetTexture(TwGetModTexture("present"))
    local presentSS = { cx = 3, cy = 1 }
    for _,p in pairs(presents) do
        local pcore = cores[p.coreID]
        if pcore then
            TwRenderSetColorF4(1, 1, 1, 1)
            SelectSprite(presentSS, 2, 0, 1, 1)
            TwRenderQuadCentered(pcore.x, pcore.y, 65, 65)

            TwRenderSetColorU32(p.color1)
            SelectSprite(presentSS, 1, 0, 1, 1)
            TwRenderQuadCentered(pcore.x, pcore.y, 65, 65)

            TwRenderSetColorU32(p.color2)
            SelectSprite(presentSS, 0, 0, 1, 1)
            TwRenderQuadCentered(pcore.x, pcore.y, 65, 65)
        end
    end

    if debug then
        DebugDraw()
    end
end

function OnSnap(packet, snapID)
    if packet.mod_id == 0x1 then
        santa = TwNetPacketUnpack(packet, {
            "float_posX",
            "float_posY",
        })
    end
    if packet.mod_id == 0x3 then
        local p = TwNetPacketUnpack(packet, {
            "i32_coreID",
            "i32_tick",
            "i32_color1",
            "i32_color2",
        })

        presents[snapID+1] = p
    end
end

function AddExplosion(exp)
    local e = {
        x = exp.posX,
        y = exp.posY,
        radius = exp.radius,
        color = exp.color,
        tstart = localtime,
        angle = (TwRandomInt(0, 1000000)/1000000.0) * pi * 2
    }

    for i = 1,#explosions,1 do
        if explosions[i] == nil then
            explosions[i] = e
            return
        end
    end

    explosions[#explosions+1] = e
end

function OnMessage(packet)
    if packet.mod_id == 0x2 then
        local exp = TwNetPacketUnpack(packet, {
            "float_posX",
            "float_posY",
            "float_radius",
            "i32_color",
        })

        print(exp)
        AddExplosion(exp)
    end
end

function OnInput(event)
    --print(event)
    if event.released and event.key == 112 then -- p
        local packet = {
            net_id = 0x1,
            force_send_now = 1,
        }
        TwNetSendPacket(packet)
        -- reset
        presents = {}
    end

    if event.released and event.key == 109 then -- m
        if debug then
            debug = false
        else
            debug = true
        end
    end
end