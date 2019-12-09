-- main mod file

function OnLoad()
    print("Example Render 1");
end

function OnRender(LocalTime, intraTick)
end

function SelectSprite(SpriteSet, x, y, w, h, flipX, flipY)
    local x1 = x/SpriteSet.cx
    local x2 = (x + w - 1/32)/SpriteSet.cx
    local y1 = y/SpriteSet.cy
    local y2 = (y + h - 1/32)/SpriteSet.cy

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

    TwRenderSetQuadSubSet( x1, y1, x2, y2 )
end

function copy(t)
    local r = {}
    for k,v in pairs(t) do
        r[k] = v
    end
    return r
end

function DrawTee(Player)
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

    --print(anim)
    --print(tee)
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

    -- front foot
    SetTexture(SkinPart.Feet)
    TwRenderSetColorF4(feetColor[1], feetColor[2], feetColor[3], feetColor[4])
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)
end

function DrawSprite(x, y, visualSize, cw, ch)
    local f = sqrt(ch*ch + cw*cw)
    local wScale = cw/f
    local hScale = ch/f
    TwRenderQuadCentered(x, y, visualSize*wScale, visualSize*hScale)
end

function DrawWeapon(Player)
    local anim = Player.anim
    local tee = Player.tee
    local Pos = Player.pos
    local Dir = Player.dir
    local weapon = Player.weapon

    local scale = tee.size/64 -- TODO: scale properly
    local gameSS = { cx = 32, cy = 16 }
    local weapSpecs = {
        [0] = { 2, 1, 4, 3,   offX = 4, offY = -20, size = 96 }, -- Hammer
        [1] = { 2, 4, 4, 2,   offX = 32, offY = -4, size = 64 }, -- Gun
        [2] = { 2, 6, 8, 2,   offX = 24, offY = -2, size = 96 }, -- Shotgun
        [3] = { 2, 8, 7, 2,   offX = 24, offY = -2, size = 96 }, -- Grenade
        [4] = { 2, 12, 7, 3,  offX = 24, offY = -2, size = 92 }, -- Laser
    }

    local weapAngleOff = {
        [1] = -3*pi/4, -- Gun
        [2] = -pi/2, -- Shotgun
        [3] = -pi/2, -- Grenade
    }

    local weapPostRotOff = {
        [1] = { x = -15, y = 4 }, -- Gun
        [2] = { x = -5, y = 4 }, -- Shotgun
        [3] = { x = -4, y = 7 }, -- Grenade
    }

    if weapon.id == 0 then -- Hammer
        local ws = weapSpecs[0]

        local posX = anim.attach.x * scale + Pos.x
        local posY = anim.attach.y * scale + Pos.y
        posY = posY + ws.offY
        if Dir.x < 0 then
            TwRenderSetQuadRotation(-pi/2 - anim.attach.angle*pi*2)
            posX = posX - ws.offX
        else
            TwRenderSetQuadRotation(-pi/2 + anim.attach.angle*pi*2)
        end

        TwRenderSetTexture(TwGetBaseTexture(Teeworlds.IMAGE_GAME))
        TwRenderSetColorF4(1, 1, 1, 1)
        
        SelectSprite(gameSS, ws[1], ws[2], ws[3], ws[4])
        DrawSprite(posX, posY, ws.size, ws[3], ws[4])

    elseif weapon.id >= 1 and weapon.id < 5 then -- Gun, Shotgun, Grenade, Laser
        local ws = weapSpecs[weapon.id]

        local posX = Pos.x + Dir.x * ws.offX - Dir.x*weapon.recoil*10
        local posY = Pos.y + Dir.y * ws.offX - Dir.y*weapon.recoil*10
        posY = posY + ws.offY
        
        TwRenderSetTexture(TwGetBaseTexture(Teeworlds.IMAGE_GAME))
        TwRenderSetColorF4(1, 1, 1, 1)
        local flipY = false
        if Dir.x < 0 then
            flipY = true
        end
        
        SelectSprite(gameSS, ws[1], ws[2], ws[3], ws[4], false, flipY)

        local angle = atan2(Dir.y, Dir.x)
        TwRenderSetQuadRotation(anim.attach.angle*pi*2 + angle)
        DrawSprite(posX, posY, ws.size, ws[3], ws[4])

        if weapon.id < 4 then
            -- draw hand
            local baseSize = tee.size/64 * 15
            local handPosX = posX + Dir.x
            local handPosY = posY + Dir.y
            
            if Dir.x < 0 then
                angle = angle - weapAngleOff[weapon.id]
            else
                angle = angle + weapAngleOff[weapon.id]
            end

            local DirX = Dir
            local DirY = { x = -Dir.y, y = Dir.x }
            if Dir.x < 0 then
                DirY.x = -DirY.x
                DirY.y = -DirY.y
            end

            local postRotOffset = weapPostRotOff[weapon.id]
            handPosX = handPosX + DirX.x * postRotOffset.x + DirY.x * postRotOffset.y
            handPosY = handPosY + DirX.y * postRotOffset.x + DirY.y * postRotOffset.y
            local handSS = { cx = 2, cy = 1 }

            TwRenderSetTexture(tee.textures[4]) -- hand texture
            
            -- outline
            TwRenderSetColorF4(1, 1, 1, 1)
            TwRenderSetQuadRotation(angle)
            SelectSprite(handSS, 1, 0, 1, 1)
            TwRenderQuadCentered(handPosX, handPosY, baseSize*2, baseSize*2)

            -- hand
            local color = tee.colors[4]
            TwRenderSetColorF4(color[1], color[2], color[3], color[4])
            TwRenderSetQuadRotation(angle)
            SelectSprite(handSS, 0, 0, 1, 1)
            TwRenderQuadCentered(handPosX, handPosY, baseSize*2, baseSize*2)
        end
    end
end

local localTime = 0

function RainbowSkin(Player)
    local tee = Player.tee

    function Color(r_, g_ ,b_)
        return {
            r = r_,
            g = g_,
            b = b_,
        }
    end

    function Palette(t, a, b, c, d)
        local r = a.r + b.r * cos(6.28318 * (c.r * t + d.r))
        local g = a.g + b.g * cos(6.28318 * (c.g * t + d.g))
        local b = a.b + b.b * cos(6.28318 * (c.b * t + d.b))
        return Color(r, g, b)
    end  

    local rainbow = Palette(localTime * 0.2, Color(0.5,0.5,0.5), Color(0.5,0.5,0.5), Color(0.8,0.8,0.8), Color(0.21,0.54,0.88))
    
    tee.colors[1] = {
        rainbow.r,
        rainbow.g,
        rainbow.b,
        1
    }

    DrawWeapon(Player)
    DrawTee(Player)
end

local flameSkin = {
    flipX = false,
    canFlip = false
}

function FlameSkin(Player)
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

    -- flame skin animation
    local flameFrameID = floor(localTime * 20) % 15

    if flameFrameID == 0 and flameSkin.canFlip then
        flameSkin.canFlip = false
        flameSkin.flipX = false
        if TwRandomInt(0, 1) == 1 then
            flameSkin.flipX = true
        end
    end

    if flameFrameID == 14 then
        flameSkin.canFlip = true
    end

    local frameIX = flameFrameID % 4
    local frameIY = floor(flameFrameID / 4)
    local frameSize = 1/4
    local x1 = frameIX * frameSize
    local x2 = (frameIX + 1) * frameSize

    if flameSkin.flipX then
        local temp = x1
        x1 = x2
        x2 = temp
    end

    local sizeVariantX = (sin(localTime * 5.2) + 1) * 0.5 * 8
    local sizeVariantY = (sin(localTime * 4.1) + 1) * 0.5 * 16
    local flameX = Player.pos.x
    local flameY = Player.pos.y - 26 - (sizeVariantY * 0.5)
    local flameW = 90 + sizeVariantX
    local flameH = 90 + sizeVariantY
    -------------

    --print(anim)
    --print(tee)
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

    -- back flame body
    TwRenderSetTexture(TwGetModTexture("flame_body_back"))
    SetColor(SkinPart.Decoration)
    TwRenderSetQuadSubSet(x1, frameIY * frameSize, x2, (frameIY + 1) * frameSize)
    TwRenderQuadCentered(flameX, flameY, flameW, flameH)

    -- front foot
    TwRenderSetTexture(TwGetModTexture("flame_feet_back"))
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 1, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)

    -- back foot
    TwRenderSetQuadRotation(backFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 1, 0, 1, 1)
    TwRenderQuadCentered(backFoot.x, backFoot.y, backFootW, backFootW)

    -- FRONT

    -- back foot
    TwRenderSetTexture(TwGetModTexture("flame_feet_back"))
    TwRenderSetColorF4(1, 0, 0, 1)
    TwRenderSetQuadRotation(backFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(backFoot.x, backFoot.y, backFootW, backFootW)

    TwRenderSetTexture(TwGetModTexture("flame_feet_front"))
    TwRenderSetColorF4(feetColor[1], feetColor[2], feetColor[3], feetColor[4])
    TwRenderSetQuadRotation(backFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(backFoot.x, backFoot.y, backFootW, backFootW)

    -- flame body
    TwRenderSetTexture(TwGetModTexture("flame_body"))
    SetColor(SkinPart.Body)
    TwRenderSetQuadSubSet(x1, frameIY * frameSize, x2, (frameIY + 1) * frameSize)
    TwRenderQuadCentered(flameX, flameY, flameW, flameH)

    TwRenderSetTexture(TwGetModTexture("flame_body_front"))
    SetColor(SkinPart.Marking)
    TwRenderSetQuadSubSet(x1, frameIY * frameSize, x2, (frameIY + 1) * frameSize)
    TwRenderQuadCentered(flameX, flameY, flameW, flameH)

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

    -- front foot
    TwRenderSetTexture(TwGetModTexture("flame_feet_back"))
    TwRenderSetColorF4(1, 0, 0, 1)
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)

    TwRenderSetTexture(TwGetModTexture("flame_feet_front"))
    TwRenderSetColorF4(feetColor[1], feetColor[2], feetColor[3], feetColor[4])
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)
end

function OnRenderPlayer(Player)
    DrawWeapon(Player)
    FlameSkin(Player)
    return true
end

function OnRender(LocalTime)
    localTime = LocalTime
end