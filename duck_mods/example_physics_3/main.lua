-- Main script file
require("utils.lua")

local isDebug = false
local game = {
    bees = {}
}

function OnLoad()
end

function DrawBee(core1, core2, scale, LocalTime)
    if not core1 or not core2 then
        return
    end

    local flipY = 1;
    if core1.x > core2.x then
        flipY = -1;
    end

    local wingAnimSS = {
        { x1= 0.5, y1= 0.5 },
        { x1= 0.5, y1= 0.0 },
        { x1= 0.0, y1= 0.5 },
        { x1= 0.0, y1= 0.0 },
    }

    TwRenderSetTexture(TwGetModTexture("bee_body"))
    TwRenderSetColorF4(1, 1, 1, 1)

    local beeDir = vec2Normalize(vec2Sub(core2, core1))
    local beeAngle = vec2Angle(beeDir)

    -- tail
    local tailDir = vec2Normalize(vec2Sub(vec2(core2.x, core2.y - (20*scale*flipY) * beeDir.x), core1))
    local tailAngle = vec2Angle(tailDir)
    local tailOffset = vec2Rotate(vec2(-8*scale, 0), tailAngle)
    local tailSizeVar = (sin(LocalTime * 5) * 2) * 0.5 * 4

    TwRenderSetQuadRotation(tailAngle)
    if flipY == 1 then
        TwRenderSetQuadSubSet(0.5, 0, 1, 0.5)
    else
        TwRenderSetQuadSubSet(0.5, 0.5, 1, 0)
    end
    TwRenderQuadCentered(core2.x + tailOffset.x, core2.y + tailOffset.y, (128*scale) + tailSizeVar, (128*scale) + tailSizeVar)

    -- middle
    TwRenderSetQuadRotation(beeAngle)
    if flipY == 1 then
        TwRenderSetQuadSubSet(0.5, 0.5, 1, 1)
    else
        TwRenderSetQuadSubSet(0.5, 1, 1, 0.5)
    end
    TwRenderQuadCentered(core1.x, core1.y, (128*scale), (128*scale))

    -- head
    local headOffset = vec2Rotate(vec2(-72*scale, 0), beeAngle)
    local headAngleVar = sin(LocalTime * 3) * pi * 0.05
    TwRenderSetQuadRotation(beeAngle + headAngleVar)
    if flipY == 1 then
        TwRenderSetQuadSubSet(0, 0, 0.5, 0.5)
    else
        TwRenderSetQuadSubSet(0, 0.5, 0.5, 0)
    end
    TwRenderQuadCentered(core1.x + headOffset.x, core1.y + headOffset.y, (128*scale), (128*scale))

    -- wing
    local wingOffset = vec2Rotate(vec2(15*scale, -55*scale*flipY), beeAngle)
    local wingSize = 150*scale
    local wingAnim = wingAnimSS[floor(LocalTime * 40) % #wingAnimSS + 1]
    TwRenderSetTexture(TwGetModTexture("bee_wing_anim"))
    if flipY == 1 then
        TwRenderSetQuadSubSet(wingAnim.x1, wingAnim.y1, wingAnim.x1 + 0.5, wingAnim.y1 + 0.5)
    else
        TwRenderSetQuadSubSet(wingAnim.x1, wingAnim.y1 + 0.5, wingAnim.x1 + 0.5, wingAnim.y1)
    end
    TwRenderSetQuadRotation(beeAngle)
    TwRenderQuadCentered(core1.x + wingOffset.x, core1.y + wingOffset.y, wingSize, wingSize)
end

function OnRender(LocalTime, IntraGameTick)
    TwRenderSetDrawSpace(0)

    TwRenderSetColorF4(1, 0, 0, 1)
    TwRenderQuad(0, 0, 500, 500)

    TwRenderSetColorF4(0, 1, 0, 1)
    TwRenderSetFreeform({
        0, 0,
        50, 0,
        0, 50,
        50, 50
    })
    TwRenderDrawFreeform(30 * 32, 30 * 32)

    TwRenderDrawText({
        text = "Hello from Lua",
        font_size = 40,
        color = {1, 1, 1, 1},
        rect = {32 * 30, 32 * 31}
    })

    TwRenderSetColorF4(1, 0.5, 1, 1)
    TwRenderDrawCircle(32 * 32, 32 * 30, 50)
    TwRenderDrawLine(32 * 32, 32 * 30, 32 * 64, 32 * 30, 10)

    local cores = TwPhysGetCores()
    local joints = TwPhysGetJoints()

    TwRenderSetDrawSpace(1)

    for k,bee in pairs(game.bees) do
        --PrintTable(bee)
        local core1 = cores[bee.core1ID+1]
        local core2 = cores[bee.core2ID+1]

        if core1 ~= nil and core2 ~= nil then
            --print(bee)
            DrawBee(core1, core2, 0.75, LocalTime)
        end
    end

    if isDebug then
        TwRenderSetDrawSpace(1)

        TwRenderSetTexture(-1)
        
        for i = 1, #cores, 1 do
            local core = cores[i]
            TwRenderSetColorF4(1, 1, 1, 1)
            TwRenderDrawCircle(core.x, core.y, core.radius)

            TwRenderSetColorF4(1, 0, 1, 0.5)
            TwRenderQuadCentered(core.x, core.y, core.radius * 2, core.radius * 2)
        end

        for i = 1, #joints, 1 do
            local joint = joints[i]
            if joint.core1_id ~= nil or joint.core2_id ~= nil then
                local core1 = cores[joint.core1_id]
                local core2 = cores[joint.core2_id]

                TwRenderSetColorF4(0, 0.2, 1, 0.85)
                TwRenderDrawLine(core1.x, core1.y, core2.x, core2.y, 10)
            end
        end
    end
end

function OnSnap(packet, snapID)
    if packet.mod_id == 0x1 then
        local bee = TwNetPacketUnpack(packet, {
            "i32_core1ID",
            "i32_core2ID",
            "i32_health",
        })

        game.bees[snapID] = bee
    end
end

function OnInput(event)
    if event.key == 112 and event.released then
        isDebug = not isDebug
    end
end