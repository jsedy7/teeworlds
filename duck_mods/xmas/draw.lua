--
require("base.lua")


function DrawSantee(x, y)
    local Player = CreateBasePlayer()

    Player.pos = {
        x = x,
        y = y,
    }
    Player.tee.size = 100
    Player.tee.colors[1] = { 1, 0.8588, 0.6705, 1 }
    Player.tee.colors[4] = { 0.9, 0, 0, 1 }
    Player.tee.colors[5] = { 0.9, 0, 0, 1 }

    local anim = Player.anim
    local tee = Player.tee
    local Pos = Player.pos
    local Dir = Player.dir
    local EmoteID = Player.emote

    local SkinPart = {
        Body = 1,
        Marking = 2,
        Decoration = 3,
        Hands = 4,
        Feet = 5,
        Eyes = 6,
    }

    function SetTexture(ID)
        TwRenderSetTexture(tee.textures[ID])
    end
    function SetColor(ID)
        local color = tee.colors[ID]
        TwRenderSetColorF4(color[1], color[2], color[3], color[4])
    end

    local body = copy(anim.body)
    local baseSize = tee.size
    local animScale = baseSize/64

    body.x = Pos.x + (body.x * animScale)
    body.y = Pos.y + (body.y * animScale)

    local bodySpriteSet = {
        cx = 2,
        cy = 2
    }

    local frontFoot = copy(anim.frontFoot)
    frontFoot.x = Pos.x + (frontFoot.x * animScale)
    frontFoot.y = Pos.y + (frontFoot.y * animScale)
    local frontFootW = baseSize/2.1

    local backFoot = copy(anim.backFoot)
    backFoot.x = Pos.x + (backFoot.x * animScale)
    backFoot.y = Pos.y + (backFoot.y * animScale)
    local backFootW = baseSize/2.1

    local feetColor = copy(tee.colors[SkinPart.Feet])
    if tee.got_airjump == 0 then
        feetColor[1] = feetColor[1] * 0.5
        feetColor[2] = feetColor[2] * 0.5
        feetColor[3] = feetColor[3] * 0.5
    end

    -- BACK

    -- back body
    SetTexture(SkinPart.Body)
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite(bodySpriteSet, 0, 0, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- decoration
    if tee.textures[SkinPart.Decoration] ~= nil then
        SetTexture(SkinPart.Decoration)
        TwRenderSetColorF4(1, 1, 1, 1)
        SelectSprite({ cx = 2, cy = 1 }, 1, 0, 1, 1)
        TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)
    end

    -- front foot
    SetTexture(SkinPart.Feet)
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 1, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)

    -- back foot
    TwRenderSetQuadRotation(backFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 1, 0, 1, 1)
    TwRenderQuadCentered(backFoot.x, backFoot.y, backFootW, backFootW)

    -- beard
    TwRenderSetTexture(TwGetModTexture("beard"))
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite({ cx = 1, cy = 2 }, 0, 1, 1, 1)
    TwRenderQuadCentered(x-0.2*baseSize, y+0.13*baseSize, baseSize, baseSize/2)


    -- FRONT

    -- back foot
    SetTexture(SkinPart.Feet)
    TwRenderSetColorF4(feetColor[1], feetColor[2], feetColor[3], feetColor[4])
    TwRenderSetQuadRotation(backFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(backFoot.x, backFoot.y, backFootW, backFootW)

    -- decoration
    if tee.textures[SkinPart.Decoration] ~= nil then
        SetTexture(SkinPart.Decoration)
        SetColor(SkinPart.Decoration)
        SelectSprite({ cx = 2, cy = 1 }, 0, 0, 1, 1)
        TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)
    end

    -- body
    SetTexture(SkinPart.Body)
    SetColor(SkinPart.Body)
    SelectSprite(bodySpriteSet, 1, 0, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- marking
    if tee.textures[SkinPart.Marking] ~= nil then
        SetTexture(SkinPart.Marking)
        SetColor(SkinPart.Marking)
        SelectSprite({ cx = 1, cy = 1 }, 0, 0, 1, 1)
        TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)
    end

    SetTexture(SkinPart.Body) -- back to body texture

    -- shadow
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite(bodySpriteSet, 0, 1, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- outline
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite(bodySpriteSet, 1, 1, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- eyes
    local Emote = {
        Normal = 0,
        Pain = 1,
        Happy = 2,
        Surprise = 3,
        Angry = 4,
        Blink = 5,
    }
    
    local emoteSS = {
        { 0, 0 },
        { 0, 1 },
        { 1, 1 },
        { 0, 2 },
        { 1, 0 },
        { 0, 0 },
    }
    
    SetTexture(SkinPart.Eyes)
    SetColor(SkinPart.Eyes)
    SelectSprite({ cx = 2, cy = 4 }, emoteSS[EmoteID+1][1], emoteSS[EmoteID+1][2], 1, 1)
    local eyeScale = baseSize*0.60
    local eyeHeight = eyeScale/2.0
    if EmoteID == Emote.Blink then
        eyeHeight = baseSize*0.15/2.0
    end

    local offsetX = (Dir.x* 0.125) * baseSize
    local offsetY = (-0.05 + Dir.y * 0.10) * baseSize
    TwRenderQuadCentered(body.x+offsetX, body.y+offsetY, eyeScale, eyeHeight)

    -- beard
    TwRenderSetTexture(TwGetModTexture("beard"))
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite({ cx = 1, cy = 2 }, 0, 0, 1, 1)
    TwRenderQuadCentered(x-0.2*baseSize, y+0.13*baseSize, baseSize, baseSize/2)

    -- front foot
    SetTexture(SkinPart.Feet)
    TwRenderSetColorF4(feetColor[1], feetColor[2], feetColor[3], feetColor[4])
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)

    -- hat
    TwRenderSetTexture(TwGetModTexture("xmas_hat"))
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite({ cx = 1, cy = 4 }, 0, 3, 1, 1, true)
    TwRenderQuadCentered(x, y-2, baseSize*1.1, baseSize*1.1)
end

function DrawReindeer(x, y, LocalTime)
    local rdPly = CreateBasePlayer()

    rdPly.pos = {
        x = x,
        y = y,
    }
    rdPly.tee.size = 100

    rdPly.tee.textures[1] = TwGetSkinPartTexture(0, "mouse")
    rdPly.tee.textures[2] = TwGetSkinPartTexture(1, "whisker")
    rdPly.tee.textures[6] = TwGetSkinPartTexture(5, "colorable")

    rdPly.tee.colors[1] = { 124/255, 74/255, 20/255, 1 }
    rdPly.tee.colors[2] = { 1, 0, 0, 1 }
    rdPly.tee.colors[5] = { 88/255, 52/255, 14/255, 1 }
    rdPly.tee.colors[6] = { 169/255, 51/255, 0, 1 }

    local s = sin(LocalTime*4)
    rdPly.anim.frontFoot.x = 3
    rdPly.anim.frontFoot.y = rdPly.anim.frontFoot.y - s * 4 + 2
    rdPly.anim.frontFoot.angle = -0.1 + s * 0.03
    rdPly.anim.backFoot.y = rdPly.anim.backFoot.y - s * 4 + 2
    rdPly.anim.backFoot.angle = -0.1 + s * 0.03

    local texAntler = TwGetModTexture("antler")
    local antlerX = rdPly.pos.x
    local antlerY = rdPly.pos.y-56
    local antlerW = 50
    local antlerH = 100
    local antlerAngle = pi*0.20

    TwRenderSetTexture(texAntler)
    TwRenderSetQuadRotation(-antlerAngle)
    SelectSprite({ cx = 2, cy = 1 }, 1, 0, 1, 1)
    TwRenderQuadCentered(antlerX-20, antlerY, antlerW, antlerH)

    TwRenderSetQuadRotation(antlerAngle)
    SelectSprite({ cx = 2, cy = 1 }, 1, 0, 1, 1, true)
    TwRenderQuadCentered(antlerX+20, antlerY, antlerW, antlerH)

    DrawTee(rdPly)

    TwRenderSetTexture(texAntler)
    TwRenderSetColorF4(82/255, 59/255, 36/255, 1)
    TwRenderSetQuadRotation(-antlerAngle)
    SelectSprite({ cx = 2, cy = 1 }, 0, 0, 1, 1)
    TwRenderQuadCentered(antlerX-20, antlerY, antlerW, antlerH)

    TwRenderSetQuadRotation(antlerAngle)
    SelectSprite({ cx = 2, cy = 1 }, 0, 0, 1, 1, true)
    TwRenderQuadCentered(antlerX+20, antlerY, antlerW, antlerH)
end