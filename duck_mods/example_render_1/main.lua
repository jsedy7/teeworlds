-- main mod file

function OnLoad()
    print("Example Render 1");
end

function OnRender(LocalTime, intraTick)
end

function SelectSprite(SpriteSet, x, y, w, h)
    local x1 = x/SpriteSet.cx
    local x2 = (x + w - 1/32)/SpriteSet.cx
    local y1 = y/SpriteSet.cy
    local y2 = (y + h - 1/32)/SpriteSet.cy
    TwRenderSetQuadSubSet( x1, y1, x2, y2 )
end

function OnRenderPlayer(AnimState, TeeInfo, Pos, ClientID)

    local SkinPart = {
        Body = 1,
        Marking = 2,
        Decoration = 3,
        Hands = 4,
        Feet = 5,
        Eyes = 6,
    }

    function SetTexture(ID)
        TwRenderSetTexture(TeeInfo.textures[ID])
    end
    function SetColor(ID)
        local color = TeeInfo.colors[ID]
        TwRenderSetColorF4(color[1], color[2], color[3], color[4])
    end

    --print(AnimState)
    --print(TeeInfo)
    local body = AnimState.body
    local baseSize = TeeInfo.size
    local animScale = baseSize/64

    body.x = Pos.x + (body.x * animScale) + 64
    body.y = Pos.y + (body.y * animScale)

    local bodySpriteSet = {
        cx = 2,
        cy = 2
    }

    local frontFoot = AnimState.frontFoot
    frontFoot.x = Pos.x + (frontFoot.x * animScale) + 64
    frontFoot.y = Pos.y + (frontFoot.y * animScale)
    local frontFootW = baseSize/2.1

    local backFoot = AnimState.backFoot
    backFoot.x = Pos.x + (backFoot.x * animScale) + 64
    backFoot.y = Pos.y + (backFoot.y * animScale)
    local backFootW = baseSize/2.1

    -- BACK

    -- back body
    SetTexture(SkinPart.Body)
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite(bodySpriteSet, 0, 0, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

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
    SetColor(SkinPart.Feet)
    TwRenderSetQuadRotation(backFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(backFoot.x, backFoot.y, backFootW, backFootW)

    -- body
    SetTexture(SkinPart.Body)
    SetColor(SkinPart.Body)
    SelectSprite(bodySpriteSet, 1, 0, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- marking
    SetTexture(SkinPart.Marking)
    SetColor(SkinPart.Marking)
    SelectSprite({ cx = 1, cy = 1 }, 0, 0, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    SetTexture(SkinPart.Body) -- back to body texture

    -- shadow
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite(bodySpriteSet, 0, 1, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- outline
    TwRenderSetColorF4(1, 1, 1, 1)
    SelectSprite(bodySpriteSet, 1, 1, 1, 1)
    TwRenderQuadCentered(body.x, body.y, baseSize, baseSize)

    -- front foot
    SetTexture(SkinPart.Feet)
    SetColor(SkinPart.Feet)
    TwRenderSetQuadRotation(frontFoot.angle*pi*2)
    SelectSprite({ cx = 2, cy = 1}, 0, 0, 1, 1)
    TwRenderQuadCentered(frontFoot.x, frontFoot.y, frontFootW, frontFootW)

    return true
end