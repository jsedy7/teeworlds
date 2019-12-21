-- main mod file
require("base.lua")
require("draw.lua")

local debug = false

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

function OnRender(LocalTime, intraTick)
    TwRenderSetDrawSpace(1)

--
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
                y = santa.y + 40 + sin(LocalTime + r) * 10,
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
            DrawReindeer(rd1.x, rd1.y)
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
end

function OnInput(event)
    --print(event)
    if event.released and event.key == 112 then -- p
        local packet = {
            net_id = 0x1,
            force_send_now = 1,
        }
        TwNetSendPacket(packet)
    end

    if event.released and event.key == 109 then -- m
        if debug then
            debug = false
        else
            debug = true
        end
    end
end