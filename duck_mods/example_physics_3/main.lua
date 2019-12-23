-- Main script file
require("utils.lua")

local isDebug = false
local game = {
    bees = {},
    beesSound = {},
    hives = {}
}

local Sprite = {
    BeeHead = { 0, 0, 0.5, 0.5 },
    BeeTail = { 0.5, 0, 1, 0.5 },
    BeeMiddle = { 0.5, 0.5, 1, 1 },
    BeeLeg1 = { 0, 0.5, 0.125, 1 },
    BeeLeg2 = { 0.125, 0.5, 0.25, 1 },
    BeeLeg3 = { 0.25, 0.5, 0.5, 1 },
}

function OnLoad()
end

function SelectSprite(spriteID, flipX, flipY)
    local sprite = Sprite[spriteID]
    local x1, y1, x2, y2 = sprite[1], sprite[2], sprite[3], sprite[4]
    if flipX then
        local temp = x1
        x1 = x2
        x2 = temp
    end
    if flipY then
        local temp = y1
        y1 = y2
        y2 = temp
    end
    TwRenderSetQuadSubSet(x1, y1, x2, y2)
end

function DrawBee(core1, core2, scale, LocalTime)
    if not core1 or not core2 then
        return
    end

    local flipY = 1;
    if core1.x > core2.x then
        flipY = -1;
    end
    local flipSpriteY = flipY ~= 1

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
    local tailSizeVar = (sin(LocalTime * 5) * 2) * 0.5 * 4 * scale

    TwRenderSetQuadRotation(tailAngle)
    SelectSprite("BeeTail", false, flipSpriteY)
    TwRenderQuadCentered(core2.x + tailOffset.x, core2.y + tailOffset.y, (128*scale) + tailSizeVar, (128*scale) + tailSizeVar)

    -- middle
    TwRenderSetQuadRotation(beeAngle)
    SelectSprite("BeeMiddle", false, flipSpriteY)
    TwRenderQuadCentered(core1.x, core1.y, (128*scale), (128*scale))

    -- head
    local headOffset = vec2Rotate(vec2(-72*scale, 0), beeAngle)
    local headAngleVar = sin(LocalTime * 3) * pi * 0.05
    TwRenderSetQuadRotation(beeAngle + headAngleVar)
    SelectSprite("BeeHead", false, flipSpriteY)
    TwRenderQuadCentered(core1.x + headOffset.x, core1.y + headOffset.y, (128*scale), (128*scale))

    -- legs
    local leg1HeightVar = -(sin(LocalTime * 2.2) + 1.0) * 0.5 * (10 * scale)
    local leg1Offset = vec2Rotate(vec2(-32*scale, (60*scale + leg1HeightVar*0.5) * flipY), beeAngle)
    local leg1AngleVar = sin(LocalTime * 2) * pi * 0.02
    TwRenderSetQuadRotation(beeAngle + leg1AngleVar)
    SelectSprite("BeeLeg1", false, flipSpriteY)
    TwRenderQuadCentered(core1.x + leg1Offset.x, core1.y + leg1Offset.y, (32*scale), (128*scale) + leg1HeightVar)

    local leg2HeightVar = -(sin((LocalTime+1) * 2.3) + 1.0) * 0.5 * (14 * scale)
    local leg2Offset = vec2Rotate(vec2(-10*scale, (50*scale + leg2HeightVar*0.5) * flipY), beeAngle)
    local leg2AngleVar = sin((LocalTime+1) * 2) * pi * 0.02
    TwRenderSetQuadRotation(beeAngle + leg2AngleVar)
    SelectSprite("BeeLeg2", false, flipSpriteY)
    TwRenderQuadCentered(core1.x + leg2Offset.x, core1.y + leg2Offset.y, (32*scale), (128*scale) + leg2HeightVar)

    local leg3HeightVar = -(sin((LocalTime+2) * 2.15) + 1.0) * 0.5 * (22 * scale)
    local leg3Offset = vec2Rotate(vec2(12*scale, (45*scale + leg3HeightVar*0.5) * flipY), beeAngle)
    local leg3AngleVar = sin((LocalTime+2) * 2) * pi * 0.02
    TwRenderSetQuadRotation(beeAngle + leg2AngleVar)
    SelectSprite("BeeLeg3", false, flipSpriteY)
    TwRenderQuadCentered(core1.x + leg3Offset.x, core1.y + leg3Offset.y, (64*scale), (128*scale) + leg3HeightVar)

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

function DrawHive(x, y, scale, LocalTime)
    TwRenderSetTexture(TwGetModTexture("hive"))
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderSetQuadSubSet(0, 0, 1, 1)

    if floor(LocalTime) % 7 < 1 then
        local angle = sin(LocalTime * 50) * pi * 0.05
        TwRenderSetQuadRotation(angle)
    end

    TwRenderQuadCentered(x, y, (128)*scale, (128)*scale)
end

local lastLocalTime = 0
local bzzSounds = {
    "bzzz-01",
    "bzzz-02",
    "bzzz-03",
    "bzzz-04",
}

function OnUpdate(LocalTime)
    local delta = LocalTime - lastLocalTime
    lastLocalTime = LocalTime

    local cores = TwPhysGetCores()

    for k,bee in pairs(game.bees) do
        local snd = game.beesSound[k]
        local core1 = cores[bee.core1ID+1]
        snd.cooldown = snd.cooldown - delta
        if snd.cooldown < 0 and core1 then
            snd.cooldown = 4.5
            local core1 = cores[bee.core1ID+1]
            local sndID = 1 + floor(LocalTime*LocalTime) % #bzzSounds
            TwPlaySoundAt(bzzSounds[sndID], core1.x, core1.y)
        end
    end
end

function OnRender(LocalTime, IntraGameTick)
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

    for k,hive in pairs(game.hives) do
        local core1 = cores[hive.coreID+1]

        if core1 ~= nil then
            --print(hive)
            DrawHive(core1.x, core1.y, 1, LocalTime)
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

        local sound = {
            cooldown = 0.0
        }
        game.bees[snapID] = bee
        if not game.beesSound[snapID] then
            game.beesSound[snapID] = sound
        end
    elseif packet.mod_id == 0x2 then
        local hive = TwNetPacketUnpack(packet, {
            "i32_coreID",
        })

        game.hives[snapID] = hive
    end
end

function OnInput(event)
    if event.key == 112 and event.released then
        isDebug = not isDebug
    end
end